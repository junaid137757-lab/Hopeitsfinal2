#include "CUnit.h"
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"
#include "income.h"
#include "common.h"

/* addIncome() calls saveTransactions(), which writes
   "<currentUser>_transactions.dat" - run from a scratch dir. */
static int suite_init(void)
{
    th_enter_tmp_dir();
    th_reset_globals();
    strcpy(currentUser, "incomeuser");
    return 0;
}

static int suite_clean(void)
{
    th_leave_tmp_dir();
    return 0;
}

/* A valid category and amount should append one Income
   transaction with the right fields, an auto-assigned id, and
   today's date. */
static void test_addIncome_valid_entry(void)
{
    th_reset_globals();
    strcpy(currentUser, "incomeuser");

    th_feed_stdin("Salary\n5000\n");
    th_capture_stdout_start();
    addIncome();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(transactionCount, 1);
    CU_ASSERT_STRING_EQUAL(transactions[0].type, "Income");
    CU_ASSERT_STRING_EQUAL(transactions[0].category, "Salary");
    CU_ASSERT_DOUBLE_EQUAL(transactions[0].amount, 5000.0, 0.001);
    CU_ASSERT_EQUAL(transactions[0].id, 1);
    CU_ASSERT(strstr(out, "Income Added Successfully") != NULL);
    free(out);
}

/* Adding a second income should get id 2 and not disturb the
   first entry - ids/count increment correctly. */
static void test_addIncome_increments_id_and_count(void)
{
    th_reset_globals();
    strcpy(currentUser, "incomeuser");

    th_feed_stdin("Salary\n5000\n");
    free(th_call_capturing(addIncome));

    th_feed_stdin("Bonus\n1000\n");
    free(th_call_capturing(addIncome));

    CU_ASSERT_EQUAL(transactionCount, 2);
    CU_ASSERT_EQUAL(transactions[1].id, 2);
    CU_ASSERT_STRING_EQUAL(transactions[1].category, "Bonus");
}

/* Entering "0" for the category should cancel without adding a
   transaction. */
static void test_addIncome_cancel_via_category(void)
{
    th_reset_globals();
    strcpy(currentUser, "incomeuser");

    th_feed_stdin("0\n");
    th_capture_stdout_start();
    addIncome();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(transactionCount, 0);
    CU_ASSERT(strstr(out, "Add Income Cancelled") != NULL);
    free(out);
}

/* Entering -1 for the amount should cancel after the category
   has already been entered, without adding a transaction. */
static void test_addIncome_cancel_via_amount(void)
{
    th_reset_globals();
    strcpy(currentUser, "incomeuser");

    th_feed_stdin("Salary\n-1\n");
    th_capture_stdout_start();
    addIncome();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(transactionCount, 0);
    CU_ASSERT(strstr(out, "Add Income Cancelled") != NULL);
    free(out);
}

/* Non-numeric amount input should be rejected by readValidFloat()
   and addIncome() should bail out without adding anything. */
static void test_addIncome_invalid_amount(void)
{
    th_reset_globals();
    strcpy(currentUser, "incomeuser");

    th_feed_stdin("Salary\nnotanumber\n");
    th_capture_stdout_start();
    addIncome();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(transactionCount, 0);
    CU_ASSERT(strstr(out, "Invalid input") != NULL);
    free(out);
}

/* When transactionCount is already at MAX, addIncome() must
   refuse to add another entry and must not consume any input. */
static void test_addIncome_transaction_limit_reached(void)
{
    th_reset_globals();
    strcpy(currentUser, "incomeuser");
    transactionCount = MAX;

    th_capture_stdout_start();
    addIncome();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(transactionCount, MAX);
    CU_ASSERT(strstr(out, "Transaction Limit Reached") != NULL);
    free(out);
}

void add_income_suite(void)
{
    CU_pSuite suite = CU_add_suite("income", suite_init, suite_clean);

    CU_add_test(suite, "addIncome records a valid entry", test_addIncome_valid_entry);
    CU_add_test(suite, "addIncome increments id and count", test_addIncome_increments_id_and_count);
    CU_add_test(suite, "addIncome cancels via category '0'", test_addIncome_cancel_via_category);
    CU_add_test(suite, "addIncome cancels via amount -1", test_addIncome_cancel_via_amount);
    CU_add_test(suite, "addIncome rejects a non-numeric amount", test_addIncome_invalid_amount);
    CU_add_test(suite, "addIncome refuses past MAX transactions", test_addIncome_transaction_limit_reached);
}
