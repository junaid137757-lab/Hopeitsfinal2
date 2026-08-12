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

void add_persistence_suite(void)
{
    CU_pSuite suite = CU_add_suite("persistence", suite_init, suite_clean);

    CU_add_test(suite, "transactions round-trip through save/load", test_transactions_round_trip);
    CU_add_test(suite, "loadTransactions resets count if no file", test_loadTransactions_missing_file);
    CU_add_test(suite, "budgets round-trip through save/load", test_budgets_round_trip);
    CU_add_test(suite, "goals round-trip through save/load", test_goals_round_trip);
    CU_add_test(suite, "data is isolated between users", test_data_isolated_per_user);
}
