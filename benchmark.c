#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "test_helpers.h"
#include "authentication.h"
#include "income.h"
#include "expense.h"
#include "transaction.h"
#include "budget.h"
#include "persistence.h"
#include "reports.h"

/* =====================================================
   benchmark.c - NOT a CUnit suite. This is a standalone
   report generator, not a pass/fail test: it measures
   memory footprint and operation timing so those numbers
   can be dropped into a "Performance & Capacity" section
   of a test sheet, separate from the CUnit pass/fail rows.

   Build (no -lcunit needed - this doesn't use CUnit at all):
       gcc common.h-consuming sources + test_helpers.c + benchmark.c -o benchmark
   See the gcc command printed by this file's header comment
   in the accompanying README/instructions.

   Numbers this program prints are specific to the machine
   it runs on - re-run it on your target hardware, don't
   quote the sandbox's numbers as your official result.
   ===================================================== */

static double ms_between(struct timespec start, struct timespec end)
{
    double sec = (double)(end.tv_sec - start.tv_sec);
    double nsec = (double)(end.tv_nsec - start.tv_nsec);
    return (sec * 1000.0) + (nsec / 1000000.0);
}

/* ---- 1. Static memory footprint -------------------------------- */

static void report_memory_footprint(void)
{
    printf("\n================ MEMORY FOOTPRINT ================\n");
    printf("sizeof(Transaction)  = %zu bytes\n", sizeof(Transaction));
    printf("sizeof(Budget)       = %zu bytes\n", sizeof(Budget));
    printf("sizeof(SavingsGoal)  = %zu bytes\n", sizeof(SavingsGoal));
    printf("sizeof(User)         = %zu bytes\n", sizeof(User));
    printf("-----------------------------------------------------\n");
    printf("transactions[%d]  = %8zu bytes\n", MAX, sizeof(Transaction) * MAX);
    printf("budgets[%d]        = %8zu bytes\n", MAX_BUDGETS, sizeof(Budget) * MAX_BUDGETS);
    printf("goals[%d]           = %8zu bytes\n", MAX_GOALS, sizeof(SavingsGoal) * MAX_GOALS);
    printf("-----------------------------------------------------\n");
    size_t total = sizeof(Transaction) * MAX + sizeof(Budget) * MAX_BUDGETS
                 + sizeof(SavingsGoal) * MAX_GOALS;
    printf("TOTAL static footprint (one loaded user) = %zu bytes (~%.1f KB)\n",
           total, (double)total / 1024.0);
    printf("NOTE: this is fixed and pre-allocated regardless of how much\n");
    printf("data the user actually has - it does NOT grow per user or\n");
    printf("scale with real usage, because storage is static arrays,\n");
    printf("not dynamic allocation. Only one user's data is resident\n");
    printf("in memory at a time (the currently logged-in user).\n");
}

/* ---- 2. Data capacity (compile-time limits) --------------------- */

static void report_capacity_limits(void)
{
    printf("\n================ CAPACITY LIMITS ==================\n");
    printf("Max transactions per user : %d  (MAX)\n", MAX);
    printf("Max budgets per user      : %d  (MAX_BUDGETS)\n", MAX_BUDGETS);
    printf("Max savings goals per user: %d  (MAX_GOALS)\n", MAX_GOALS);
    printf("Max user accounts total   : unbounded by code - limited only\n");
    printf("                            by disk space (users.dat grows by\n");
    printf("                            %zu bytes per registered account)\n",
           sizeof(User));
    printf("These are hard compile-time caps: addIncome/addExpense/\n");
    printf("setBudget/setSavingsGoal refuse once the count reaches the\n");
    printf("limit (see the 'refuses past MAX' CUnit tests).\n");
}

/* ---- helpers for seeding data directly (bypassing stdin) -------- */

static void seed_transactions(int n)
{
    int i;
    const char *categories[] = { "Food", "Rent", "Travel", "Salary", "Utilities" };
    const char *types[] = { "Expense", "Income" };

    transactionCount = 0;

    for(i = 0; i < n; i++)
    {
        Transaction *t = &transactions[i];
        t->id = i + 1;
        strcpy(t->type, types[i % 2]);
        strcpy(t->category, categories[i % 5]);
        t->amount = 10.0f + (float)(i % 500);
        strcpy(t->date, "2026-01-01");
    }
    transactionCount = n;
}

/* ---- 3. Operation speed at scale --------------------------------- */

static void benchmark_at_scale(int n)
{
    struct timespec t0, t1;
    int reps = 10;
    int r;
    double save_total = 0, load_total = 0, search_total = 0;
    double view_total = 0, catrep_total = 0, monrep_total = 0;
    int loaded_count = 0;

    th_reset_globals();
    strcpy(currentUser, "benchuser");

    printf("\n--- N = %d transactions (avg of %d reps) ---\n", n, reps);

    for(r = 0; r < reps; r++)
    {
        seed_transactions(n);

        clock_gettime(CLOCK_MONOTONIC, &t0);
        saveTransactions();
        clock_gettime(CLOCK_MONOTONIC, &t1);
        save_total += ms_between(t0, t1);

        transactionCount = 0;
        memset(transactions, 0, sizeof(transactions));
        clock_gettime(CLOCK_MONOTONIC, &t0);
        loadTransactions();
        clock_gettime(CLOCK_MONOTONIC, &t1);
        load_total += ms_between(t0, t1);
        loaded_count = transactionCount;

        th_feed_stdin("Utilities\n");
        clock_gettime(CLOCK_MONOTONIC, &t0);
        free(th_call_capturing(searchTransactionsByCategory));
        clock_gettime(CLOCK_MONOTONIC, &t1);
        search_total += ms_between(t0, t1);

        clock_gettime(CLOCK_MONOTONIC, &t0);
        free(th_call_capturing(viewTransactions));
        clock_gettime(CLOCK_MONOTONIC, &t1);
        view_total += ms_between(t0, t1);

        clock_gettime(CLOCK_MONOTONIC, &t0);
        free(th_call_capturing(categoryWiseReport));
        clock_gettime(CLOCK_MONOTONIC, &t1);
        catrep_total += ms_between(t0, t1);

        clock_gettime(CLOCK_MONOTONIC, &t0);
        free(th_call_capturing(monthlyReport));
        clock_gettime(CLOCK_MONOTONIC, &t1);
        monrep_total += ms_between(t0, t1);
    }

    printf("  saveTransactions()             : %8.4f ms\n", save_total / reps);
    printf("  loadTransactions()             : %8.4f ms  (loaded %d)\n",
           load_total / reps, loaded_count);
    printf("  searchTransactionsByCategory() : %8.4f ms\n", search_total / reps);
    printf("  viewTransactions()             : %8.4f ms\n", view_total / reps);
    printf("  categoryWiseReport()           : %8.4f ms\n", catrep_total / reps);
    printf("  monthlyReport()                : %8.4f ms\n", monrep_total / reps);
}

/* ---- 4. addExpense() through the real interactive path ----------
   This includes the stdin-feed harness overhead, so it's not a pure
   measurement of addExpense() itself - it's here to show the number
   is small enough that, in real usage, a human typing is always the
   bottleneck, never the program logic. */

static void benchmark_interactive_add(int reps)
{
    struct timespec t0, t1;
    int i;

    th_reset_globals();
    strcpy(currentUser, "benchuser");

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for(i = 0; i < reps; i++)
    {
        th_feed_stdin("Food\n25\n");
        free(th_call_capturing(addExpense));
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    printf("\n--- addExpense() via interactive input, %d reps ---\n", reps);
    printf("  total : %8.3f ms   avg/call : %8.4f ms\n",
           ms_between(t0, t1), ms_between(t0, t1) / (double)reps);
}

/* ---- 5. User-account scale: does login slow down as the account
   count grows? users.dat is scanned linearly in loginUser(), so
   this should show roughly linear growth with K. -------------- */

static void seed_user_accounts(int k)
{
    FILE *fp = fopen("users.dat", "wb");
    int i;

    for(i = 0; i < k; i++)
    {
        User u;
        char plainPassword[30];

        snprintf(u.username, sizeof(u.username), "user%06d", i);
        snprintf(plainPassword, sizeof(plainPassword), "Password%d!", i);
        (void)sha256GenerateSaltHex(u.saltHex);
        sha256HashSalted(u.saltHex, plainPassword, u.passwordHash);
        fwrite(&u, sizeof(User), 1, fp);
    }
    fclose(fp);
}

static void benchmark_user_scale(int k)
{
    struct timespec t0, t1;
    char username[30];
    char stdin_input[64];
    int reps = 20;
    int i;
    double total_ms = 0.0;
    int result = 0;

    th_reset_globals();
    seed_user_accounts(k);

    /* worst case: log in as the LAST account written, forcing a
       full linear scan of all k records first. Repeated `reps`
       times and averaged, since a single run at microsecond scale
       is too noisy to trust on its own. */
    snprintf(username, sizeof(username), "user%06d", k - 1);
    snprintf(stdin_input, sizeof(stdin_input), "%s\nPassword%d!\n", username, k - 1);

    for(i = 0; i < reps; i++)
    {
        th_feed_stdin(stdin_input);
        th_capture_stdout_start();
        clock_gettime(CLOCK_MONOTONIC, &t0);
        result = loginUser();
        clock_gettime(CLOCK_MONOTONIC, &t1);
        free(th_capture_stdout_end());
        total_ms += ms_between(t0, t1);
    }

    printf("  K = %6d accounts : loginUser() (worst case, avg of %d) = %8.4f ms  (result=%d)\n",
           k, reps, total_ms / (double)reps, result);
}

int main(void)
{
    report_memory_footprint();
    report_capacity_limits();

    th_enter_tmp_dir();

    printf("\n================ OPERATION SPEED AT SCALE ==========\n");
    benchmark_at_scale(10);
    benchmark_at_scale(100);
    benchmark_at_scale(500);
    benchmark_at_scale(MAX);

    th_leave_tmp_dir();
    th_enter_tmp_dir();
    benchmark_interactive_add(50);
    th_leave_tmp_dir();

    printf("\n================ USER ACCOUNT SCALE =================\n");
    th_enter_tmp_dir();
    benchmark_user_scale(10);
    th_leave_tmp_dir();

    th_enter_tmp_dir();
    benchmark_user_scale(100);
    th_leave_tmp_dir();

    th_enter_tmp_dir();
    benchmark_user_scale(1000);
    th_leave_tmp_dir();

    printf("\n=====================================================\n");
    printf("Done. Numbers above are for THIS machine - re-run on\n");
    printf("your target hardware before recording official figures.\n");

    return 0;
}
