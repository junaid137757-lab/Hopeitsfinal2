#include "CUnit.h"
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"
#include "dashboard.h"
#include "common.h"

static int suite_init(void)
{
    th_reset_globals();
    return 0;
}

static int suite_clean(void)
{
    return 0;
}

/* The dashboard should total income/expense, compute net savings,
   and count over-budget categories correctly. */
static void test_dashboard_totals_are_correct(void)
{
    th_reset_globals();
    strcpy(currentUser, "dashuser");

    transactions[0].id = 1;
    strcpy(transactions[0].type, "Income");
    strcpy(transactions[0].category, "Salary");
    transactions[0].amount = 4000.0f;
    strcpy(transactions[0].date, "2026-01-01");

    transactions[1].id = 2;
    strcpy(transactions[1].type, "Expense");
    strcpy(transactions[1].category, "Food");
    transactions[1].amount = 600.0f;
    strcpy(transactions[1].date, "2026-01-02");
    transactionCount = 2;

    strcpy(budgets[0].category, "Food");
    budgets[0].limit = 500.0f; /* 600 spent > 500 limit -> over */
    budgetCount = 1;

    goalCount = 1;

    th_capture_stdout_start();
    financialDashboard();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "dashuser") != NULL);
    CU_ASSERT(strstr(out, "4000.00") != NULL); /* total income */
    CU_ASSERT(strstr(out, "600.00") != NULL);  /* total expense */
    CU_ASSERT(strstr(out, "3400.00") != NULL); /* net savings */
    CU_ASSERT(strstr(out, "1 (1 over limit)") != NULL);
    CU_ASSERT(strstr(out, "Savings Goals    : 1") != NULL);
    free(out);
}

/* With zero income, the dashboard must skip the "Savings Rate"
   line entirely rather than divide by zero. */
static void test_dashboard_skips_savings_rate_with_no_income(void)
{
    th_reset_globals();
    strcpy(currentUser, "dashuser");

    transactions[0].id = 1;
    strcpy(transactions[0].type, "Expense");
    strcpy(transactions[0].category, "Food");
    transactions[0].amount = 100.0f;
    strcpy(transactions[0].date, "2026-01-01");
    transactionCount = 1;

    th_capture_stdout_start();
    financialDashboard();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Savings Rate") == NULL);
    free(out);
}

void add_dashboard_suite(void)
{
    CU_pSuite suite = CU_add_suite("dashboard", suite_init, suite_clean);

    CU_add_test(suite, "dashboard computes totals and over-budget count", test_dashboard_totals_are_correct);
    CU_add_test(suite, "dashboard skips savings rate with no income", test_dashboard_skips_savings_rate_with_no_income);
}
