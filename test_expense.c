#include "CUnit.h"
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"
#include "expense.h"
#include "category_index.h"
#include "common.h"

static int suite_init(void)
{
    th_enter_tmp_dir();
    th_reset_globals();
    strcpy(currentUser, "expenseuser");
    return 0;
}

static int suite_clean(void)
{
    th_leave_tmp_dir();
    return 0;
}

/* A valid category and amount should append one Expense
   transaction with the right fields and an auto-assigned id. */
static void test_addExpense_valid_entry(void)
{
    th_reset_globals();
    strcpy(currentUser, "expenseuser");

    th_feed_stdin("Groceries\n120\n");
    th_capture_stdout_start();
    addExpense();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(transactionCount, 1);
    CU_ASSERT_STRING_EQUAL(transactions[0].type, "Expense");
    CU_ASSERT_STRING_EQUAL(transactions[0].category, "Groceries");
    CU_ASSERT_DOUBLE_EQUAL(transactions[0].amount, 120.0, 0.001);
    CU_ASSERT(strstr(out, "Expense Added Successfully") != NULL);
    free(out);
}

/* Entering "0" for the category should cancel without adding a
   transaction. */
static void test_addExpense_cancel_via_category(void)
{
    th_reset_globals();
    strcpy(currentUser, "expenseuser");

    th_feed_stdin("0\n");
    th_capture_stdout_start();
    addExpense();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(transactionCount, 0);
    CU_ASSERT(strstr(out, "Add Expense Cancelled") != NULL);
    free(out);
}

/* Entering -1 for the amount should cancel without adding a
   transaction. */
static void test_addExpense_cancel_via_amount(void)
{
    th_reset_globals();
    strcpy(currentUser, "expenseuser");

    th_feed_stdin("Groceries\n-1\n");
    th_capture_stdout_start();
    addExpense();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(transactionCount, 0);
    CU_ASSERT(strstr(out, "Add Expense Cancelled") != NULL);
    free(out);
}

/* An expense that pushes a category over its budget limit should
   trigger the [ALERT] via checkBudgetAlert(), on top of the
   normal "Expense Added Successfully" message. */
static void test_addExpense_triggers_budget_alert(void)
{
    th_reset_globals();
    strcpy(currentUser, "expenseuser");

    strcpy(budgets[0].category, "Food");
    budgets[0].limit = 100.0f;
    budgetCount = 1;
    categoryIndexRebuild();

    th_feed_stdin("Food\n150\n");
    th_capture_stdout_start();
    addExpense();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Expense Added Successfully") != NULL);
    CU_ASSERT(strstr(out, "[ALERT]") != NULL);
    free(out);
}

/* An expense well within budget should not print any alert. */
static void test_addExpense_no_alert_within_budget(void)
{
    th_reset_globals();
    strcpy(currentUser, "expenseuser");

    strcpy(budgets[0].category, "Food");
    budgets[0].limit = 1000.0f;
    budgetCount = 1;
    categoryIndexRebuild();

    th_feed_stdin("Food\n50\n");
    th_capture_stdout_start();
    addExpense();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "[ALERT]") == NULL);
    CU_ASSERT(strstr(out, "[WARNING]") == NULL);
    free(out);
}

/* When transactionCount is already at MAX, addExpense() must
   refuse to add another entry. */
static void test_addExpense_transaction_limit_reached(void)
{
    th_reset_globals();
    strcpy(currentUser, "expenseuser");
    transactionCount = MAX;

    th_capture_stdout_start();
    addExpense();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(transactionCount, MAX);
    CU_ASSERT(strstr(out, "Transaction Limit Reached") != NULL);
    free(out);
}

void add_expense_suite(void)
{
    CU_pSuite suite = CU_add_suite("expense", suite_init, suite_clean);

    CU_add_test(suite, "addExpense records a valid entry", test_addExpense_valid_entry);
    CU_add_test(suite, "addExpense cancels via category '0'", test_addExpense_cancel_via_category);
    CU_add_test(suite, "addExpense cancels via amount -1", test_addExpense_cancel_via_amount);
    CU_add_test(suite, "addExpense triggers budget alert when over limit", test_addExpense_triggers_budget_alert);
    CU_add_test(suite, "addExpense stays quiet within budget", test_addExpense_no_alert_within_budget);
    CU_add_test(suite, "addExpense refuses past MAX transactions", test_addExpense_transaction_limit_reached);
}
