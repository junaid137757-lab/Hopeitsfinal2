#include "CUnit.h"
#include <string.h>

#include "test_helpers.h"
#include "persistence.h"
#include "common.h"

/* Every test in this suite runs from its own scratch temp
   directory so the *.dat files these functions create/read never
   touch real project files or leak between tests. */
static int suite_init(void)
{
    th_enter_tmp_dir();
    th_reset_globals();
    strcpy(currentUser, "persistuser");
    return 0;
}

static int suite_clean(void)
{
    th_leave_tmp_dir();
    return 0;
}

/* Saving then loading transactions (after clearing memory) should
   restore exactly what was saved. */
static void test_transactions_round_trip(void)
{
    th_reset_globals();
    strcpy(currentUser, "persistuser");

    transactions[0].id = 1;
    strcpy(transactions[0].type, "Income");
    strcpy(transactions[0].category, "Salary");
    transactions[0].amount = 5000.0f;
    strcpy(transactions[0].date, "2026-01-01");

    transactions[1].id = 2;
    strcpy(transactions[1].type, "Expense");
    strcpy(transactions[1].category, "Rent");
    transactions[1].amount = 1200.0f;
    strcpy(transactions[1].date, "2026-01-02");

    transactionCount = 2;

    saveTransactions();

    transactionCount = 0;
    memset(transactions, 0, sizeof(transactions));

    loadTransactions();

    CU_ASSERT_EQUAL(transactionCount, 2);
    CU_ASSERT_STRING_EQUAL(transactions[0].category, "Salary");
    CU_ASSERT_DOUBLE_EQUAL(transactions[0].amount, 5000.0, 0.001);
    CU_ASSERT_STRING_EQUAL(transactions[1].category, "Rent");
    CU_ASSERT_DOUBLE_EQUAL(transactions[1].amount, 1200.0, 0.001);
}

/* Loading transactions for a user that has no data file yet
   should reset the count to 0 rather than crash or leave stale
   data in place. */
static void test_loadTransactions_missing_file(void)
{
    th_reset_globals();
    strcpy(currentUser, "nobody_has_this_file");

    transactionCount = 99; /* stale value that must be reset */

    loadTransactions();

    CU_ASSERT_EQUAL(transactionCount, 0);
}

/* Saving then loading budgets should round-trip correctly. */
static void test_budgets_round_trip(void)
{
    th_reset_globals();
    strcpy(currentUser, "persistuser");

    strcpy(budgets[0].category, "Food");
    budgets[0].limit = 300.0f;
    strcpy(budgets[1].category, "Travel");
    budgets[1].limit = 500.0f;
    budgetCount = 2;

    saveBudgets();

    budgetCount = 0;
    memset(budgets, 0, sizeof(budgets));

    loadBudgets();

    CU_ASSERT_EQUAL(budgetCount, 2);
    CU_ASSERT_STRING_EQUAL(budgets[0].category, "Food");
    CU_ASSERT_DOUBLE_EQUAL(budgets[1].limit, 500.0, 0.001);
}

/* Saving then loading savings goals should round-trip correctly,
   including the saved-so-far amount. */
static void test_goals_round_trip(void)
{
    th_reset_globals();
    strcpy(currentUser, "persistuser");

    strcpy(goals[0].name, "Emergency Fund");
    goals[0].targetAmount = 10000.0f;
    goals[0].savedAmount = 2500.0f;
    goalCount = 1;

    saveGoals();

    goalCount = 0;
    memset(goals, 0, sizeof(goals));

    loadGoals();

    CU_ASSERT_EQUAL(goalCount, 1);
    CU_ASSERT_STRING_EQUAL(goals[0].name, "Emergency Fund");
    CU_ASSERT_DOUBLE_EQUAL(goals[0].targetAmount, 10000.0, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(goals[0].savedAmount, 2500.0, 0.001);
}

/* Two different usernames must not share data - persistence is
   per-user via distinct filenames. */
static void test_data_isolated_per_user(void)
{
    th_reset_globals();
    strcpy(currentUser, "alice");
    strcpy(budgets[0].category, "Food");
    budgets[0].limit = 111.0f;
    budgetCount = 1;
    saveBudgets();

    th_reset_globals();
    strcpy(currentUser, "bob");

    loadBudgets();

    CU_ASSERT_EQUAL(budgetCount, 0);
}

/* ---- corrupted/short-file and write-failure paths --------------
   The four load*() functions each have two defensive branches
   besides "file missing": the file exists but is too short to
   even hold the record count, or the count says N records but
   the file has fewer bytes than that. The four save*() functions
   each have one: fopen() for writing fails. None of these are
   reachable through a normal round-trip, so each is triggered
   here directly - either by hand-writing a truncated file at the
   exact path load*() expects, or by pointing currentUser at a
   path whose parent directory doesn't exist so the write fails. */

static void writeRawFile(const char *path, const void *data, size_t bytes)
{
    FILE *fp = fopen(path, "wb");

    if(fp != NULL)
    {
        (void)fwrite(data, 1, bytes, fp);
        (void)fclose(fp);
    }
}

static void test_loadTransactions_corrupted_count(void)
{
    char partial[2] = { 0x01, 0x00 }; /* shorter than sizeof(int) */

    th_reset_globals();
    strcpy(currentUser, "corruptuser");
    transactionCount = 99;

    writeRawFile("data/corruptuser_transactions.dat", partial, sizeof(partial));

    loadTransactions();

    CU_ASSERT_EQUAL(transactionCount, 0);
}

static void test_loadTransactions_corrupted_records(void)
{
    int count = 3; /* claims 3 records but the file has none */

    th_reset_globals();
    strcpy(currentUser, "corruptuser2");
    transactionCount = 99;

    writeRawFile("data/corruptuser2_transactions.dat", &count, sizeof(count));

    loadTransactions();

    CU_ASSERT_EQUAL(transactionCount, 0);
}

static void test_saveTransactions_write_failure(void)
{
    FILE *check;

    th_reset_globals();
    strcpy(currentUser, "nosuchdir/x");
    transactionCount = 1;
    strcpy(transactions[0].type, "Income");
    strcpy(transactions[0].category, "Test");
    transactions[0].amount = 1.0f;

    saveTransactions(); /* must not crash even though the write fails */

    check = fopen("data/nosuchdir/x_transactions.dat", "rb");
    CU_ASSERT_PTR_NULL(check);

    if(check != NULL)
    {
        fclose(check);
    }
}

static void test_loadBudgets_corrupted_count(void)
{
    char partial[1] = { 0x01 };

    th_reset_globals();
    strcpy(currentUser, "corruptuser3");
    budgetCount = 99;

    writeRawFile("data/corruptuser3_budgets.dat", partial, sizeof(partial));

    loadBudgets();

    CU_ASSERT_EQUAL(budgetCount, 0);
}

static void test_loadBudgets_corrupted_records(void)
{
    int count = 2;

    th_reset_globals();
    strcpy(currentUser, "corruptuser4");
    budgetCount = 99;

    writeRawFile("data/corruptuser4_budgets.dat", &count, sizeof(count));

    loadBudgets();

    CU_ASSERT_EQUAL(budgetCount, 0);
}

static void test_saveBudgets_write_failure(void)
{
    FILE *check;

    th_reset_globals();
    strcpy(currentUser, "nosuchdir/y");
    budgetCount = 1;
    strcpy(budgets[0].category, "Test");
    budgets[0].limit = 1.0f;

    saveBudgets();

    check = fopen("data/nosuchdir/y_budgets.dat", "rb");
    CU_ASSERT_PTR_NULL(check);

    if(check != NULL)
    {
        fclose(check);
    }
}

static void test_loadGoals_missing_file(void)
{
    th_reset_globals();
    strcpy(currentUser, "nobody_has_goals");
    goalCount = 99;

    loadGoals();

    CU_ASSERT_EQUAL(goalCount, 0);
}

static void test_loadGoals_corrupted_count(void)
{
    char partial[1] = { 0x01 };

    th_reset_globals();
    strcpy(currentUser, "corruptuser5");
    goalCount = 99;

    writeRawFile("data/corruptuser5_goals.dat", partial, sizeof(partial));

    loadGoals();

    CU_ASSERT_EQUAL(goalCount, 0);
}

static void test_loadGoals_corrupted_records(void)
{
    int count = 4;

    th_reset_globals();
    strcpy(currentUser, "corruptuser6");
    goalCount = 99;

    writeRawFile("data/corruptuser6_goals.dat", &count, sizeof(count));

    loadGoals();

    CU_ASSERT_EQUAL(goalCount, 0);
}

static void test_saveGoals_write_failure(void)
{
    FILE *check;

    th_reset_globals();
    strcpy(currentUser, "nosuchdir/z");
    goalCount = 1;
    strcpy(goals[0].name, "Test");
    goals[0].targetAmount = 1.0f;
    goals[0].savedAmount = 0.0f;

    saveGoals();

    check = fopen("data/nosuchdir/z_goals.dat", "rb");
    CU_ASSERT_PTR_NULL(check);

    if(check != NULL)
    {
        fclose(check);
    }
}

static void test_loadEmergencyFund_missing_file(void)
{
    th_reset_globals();
    strcpy(currentUser, "nobody_has_a_fund");
    emergencyFundBalance = 12345.0f;

    loadEmergencyFund();

    CU_ASSERT_DOUBLE_EQUAL(emergencyFundBalance, 0.0, 0.001);
}

static void test_loadEmergencyFund_corrupted(void)
{
    char partial[1] = { 0x01 }; /* shorter than sizeof(float) */

    th_reset_globals();
    strcpy(currentUser, "corruptuser7");
    emergencyFundBalance = 12345.0f;

    writeRawFile("data/corruptuser7_emergencyfund.dat", partial, sizeof(partial));

    loadEmergencyFund();

    CU_ASSERT_DOUBLE_EQUAL(emergencyFundBalance, 0.0, 0.001);
}

static void test_saveEmergencyFund_write_failure(void)
{
    FILE *check;

    th_reset_globals();
    strcpy(currentUser, "nosuchdir/w");
    emergencyFundBalance = 500.0f;

    saveEmergencyFund();

    check = fopen("data/nosuchdir/w_emergencyfund.dat", "rb");
    CU_ASSERT_PTR_NULL(check);

    if(check != NULL)
    {
        fclose(check);
    }
}

void add_persistence_suite(void)
{
    CU_pSuite suite = CU_add_suite("persistence", suite_init, suite_clean);

    CU_add_test(suite, "transactions round-trip through save/load", test_transactions_round_trip);
    CU_add_test(suite, "loadTransactions resets count if no file", test_loadTransactions_missing_file);
    CU_add_test(suite, "budgets round-trip through save/load", test_budgets_round_trip);
    CU_add_test(suite, "goals round-trip through save/load", test_goals_round_trip);
    CU_add_test(suite, "data is isolated between users", test_data_isolated_per_user);

    CU_add_test(suite, "loadTransactions handles a truncated count field", test_loadTransactions_corrupted_count);
    CU_add_test(suite, "loadTransactions handles truncated records", test_loadTransactions_corrupted_records);
    CU_add_test(suite, "saveTransactions handles an unwritable path", test_saveTransactions_write_failure);

    CU_add_test(suite, "loadBudgets handles a truncated count field", test_loadBudgets_corrupted_count);
    CU_add_test(suite, "loadBudgets handles truncated records", test_loadBudgets_corrupted_records);
    CU_add_test(suite, "saveBudgets handles an unwritable path", test_saveBudgets_write_failure);

    CU_add_test(suite, "loadGoals resets count if no file", test_loadGoals_missing_file);
    CU_add_test(suite, "loadGoals handles a truncated count field", test_loadGoals_corrupted_count);
    CU_add_test(suite, "loadGoals handles truncated records", test_loadGoals_corrupted_records);
    CU_add_test(suite, "saveGoals handles an unwritable path", test_saveGoals_write_failure);

    CU_add_test(suite, "loadEmergencyFund resets balance if no file", test_loadEmergencyFund_missing_file);
    CU_add_test(suite, "loadEmergencyFund handles a truncated file", test_loadEmergencyFund_corrupted);
    CU_add_test(suite, "saveEmergencyFund handles an unwritable path", test_saveEmergencyFund_write_failure);
}
