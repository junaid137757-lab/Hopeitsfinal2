#include "CUnit.h"
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"
#include "investment.h"
#include "persistence.h"
#include "common.h"

/* investmentMenu() is the only public entry point - everything
   else in investment.c is static. Tests drive it through full
   menu navigation sequences (mirroring how main.c's own menu is
   exercised) rather than calling internal helpers directly.
   Emergency Fund deposit/withdraw writes <user>_emergencyfund.dat,
   so this suite runs from its own scratch temp directory. */

static int suite_init(void)
{
    th_enter_tmp_dir();
    th_reset_globals();
    strcpy(currentUser, "investuser");
    emergencyFundBalance = 0;
    return 0;
}

static int suite_clean(void)
{
    th_leave_tmp_dir();
    return 0;
}

static void reset(void)
{
    th_reset_globals();
    strcpy(currentUser, "investuser");
    emergencyFundBalance = 0;
}

/* Viewing Mutual Funds should show its info and the risk
   disclaimer (it's market-linked). */
static void test_view_mutual_funds_shows_disclaimer(void)
{
    reset();

    /* Investment menu -> View Categories -> Mutual Funds -> Back -> Back */
    th_feed_stdin("1\n1\n5\n3\n");
    th_capture_stdout_start();
    investmentMenu();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Mutual Funds") != NULL);
    CU_ASSERT(strstr(out, "Groww") != NULL);
    CU_ASSERT(strstr(out, "3-5 years") != NULL);
    CU_ASSERT(strstr(out, "Disclaimer") != NULL);
    free(out);
}

/* Viewing Gold should also show the disclaimer (also market-linked). */
static void test_view_gold_shows_disclaimer(void)
{
    reset();

    th_feed_stdin("1\n2\n5\n3\n");
    th_capture_stdout_start();
    investmentMenu();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Gold") != NULL);
    CU_ASSERT(strstr(out, "Paytm Gold") != NULL);
    CU_ASSERT(strstr(out, "Disclaimer") != NULL);
    free(out);
}

/* Health Insurance is not a market investment - no disclaimer
   should be shown for it. */
static void test_view_health_insurance_no_disclaimer(void)
{
    reset();

    th_feed_stdin("1\n3\n5\n3\n");
    th_capture_stdout_start();
    investmentMenu();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Health Insurance") != NULL);
    CU_ASSERT(strstr(out, "PolicyBazaar") != NULL);
    CU_ASSERT(strstr(out, "Disclaimer") == NULL);
    free(out);
}

/* Selecting Emergency Funds and depositing should update the
   balance and report success, with no disclaimer (not market-linked). */
static void test_emergency_fund_deposit(void)
{
    reset();

    /* Menu -> Categories -> Emergency Funds -> Deposit -> 5000 -> Back -> Back -> Back */
    th_feed_stdin("1\n4\n1\n5000\n3\n5\n3\n");
    th_capture_stdout_start();
    investmentMenu();
    char *out = th_capture_stdout_end();

    CU_ASSERT_DOUBLE_EQUAL(emergencyFundBalance, 5000.0, 0.001);
    CU_ASSERT(strstr(out, "Deposited Successfully") != NULL);
    CU_ASSERT(strstr(out, "Disclaimer") == NULL);
    free(out);
}

/* A deposit made through the menu should actually persist to disk -
   reloading it via loadEmergencyFund() after zeroing the in-memory
   value should bring the deposited amount back. */
static void test_emergency_fund_deposit_persists(void)
{
    reset();

    th_feed_stdin("1\n4\n1\n2500\n3\n5\n3\n");
    free(th_call_capturing(investmentMenu));

    emergencyFundBalance = 0;
    loadEmergencyFund();

    CU_ASSERT_DOUBLE_EQUAL(emergencyFundBalance, 2500.0, 0.001);
}

/* Withdrawing more than the current balance must be rejected and
   leave the balance unchanged. */
static void test_emergency_fund_withdraw_insufficient(void)
{
    reset();
    emergencyFundBalance = 100.0f;

    th_feed_stdin("1\n4\n2\n500\n3\n5\n3\n");
    th_capture_stdout_start();
    investmentMenu();
    char *out = th_capture_stdout_end();

    CU_ASSERT_DOUBLE_EQUAL(emergencyFundBalance, 100.0, 0.001);
    CU_ASSERT(strstr(out, "Insufficient Balance") != NULL);
    free(out);
}

/* A valid withdrawal within the balance should succeed and reduce it. */
static void test_emergency_fund_withdraw_success(void)
{
    reset();
    emergencyFundBalance = 1000.0f;

    th_feed_stdin("1\n4\n2\n400\n3\n5\n3\n");
    th_capture_stdout_start();
    investmentMenu();
    char *out = th_capture_stdout_end();

    CU_ASSERT_DOUBLE_EQUAL(emergencyFundBalance, 600.0, 0.001);
    CU_ASSERT(strstr(out, "Withdrawn Successfully") != NULL);
    free(out);
}

/* Depositing 0 (or a negative amount other than the -1 cancel
   sentinel) should be rejected without changing the balance. */
static void test_emergency_fund_deposit_rejects_zero(void)
{
    reset();

    th_feed_stdin("1\n4\n1\n0\n3\n5\n3\n");
    th_capture_stdout_start();
    investmentMenu();
    char *out = th_capture_stdout_end();

    CU_ASSERT_DOUBLE_EQUAL(emergencyFundBalance, 0.0, 0.001);
    CU_ASSERT(strstr(out, "must be greater than zero") != NULL);
    free(out);
}

/* Entering -1 for the deposit amount should cancel it. */
static void test_emergency_fund_deposit_cancel(void)
{
    reset();

    th_feed_stdin("1\n4\n1\n-1\n3\n5\n3\n");
    th_capture_stdout_start();
    investmentMenu();
    char *out = th_capture_stdout_end();

    CU_ASSERT_DOUBLE_EQUAL(emergencyFundBalance, 0.0, 0.001);
    CU_ASSERT(strstr(out, "Deposit Cancelled") != NULL);
    free(out);
}

/* Get Recommendation should split the amount across all four
   categories using the documented percentages, and show the
   disclaimer since Mutual Funds/Gold are included. */
static void test_recommendation_valid_amount(void)
{
    reset();

    th_feed_stdin("2\n10000\n3\n");
    th_capture_stdout_start();
    investmentMenu();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Suggested Allocation for 10000.00") != NULL);
    CU_ASSERT(strstr(out, "5000.00") != NULL); /* Mutual Funds 50% */
    CU_ASSERT(strstr(out, "2000.00") != NULL); /* Gold or Emergency Fund 20% */
    CU_ASSERT(strstr(out, "1000.00") != NULL); /* Health Insurance 10% */
    CU_ASSERT(strstr(out, "Disclaimer") != NULL);
    free(out);
}

/* Entering -1 for the recommendation amount should cancel it. */
static void test_recommendation_cancel(void)
{
    reset();

    th_feed_stdin("2\n-1\n3\n");
    th_capture_stdout_start();
    investmentMenu();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Cancelled") != NULL);
    CU_ASSERT(strstr(out, "Suggested Allocation") == NULL);
    free(out);
}

/* A zero or negative amount (other than -1) should be rejected. */
static void test_recommendation_rejects_zero(void)
{
    reset();

    th_feed_stdin("2\n0\n3\n");
    th_capture_stdout_start();
    investmentMenu();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "must be greater than zero") != NULL);
    free(out);
}

/* An out-of-range menu choice should print "Invalid Choice" and
   let the user try again rather than crashing or exiting silently. */
static void test_invalid_menu_choice(void)
{
    reset();

    th_feed_stdin("9\n3\n");
    th_capture_stdout_start();
    investmentMenu();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Invalid Choice") != NULL);
    free(out);
}

void add_investment_suite(void)
{
    CU_pSuite suite = CU_add_suite("investment", suite_init, suite_clean);

    CU_add_test(suite, "viewing Mutual Funds shows the disclaimer", test_view_mutual_funds_shows_disclaimer);
    CU_add_test(suite, "viewing Gold shows the disclaimer", test_view_gold_shows_disclaimer);
    CU_add_test(suite, "viewing Health Insurance shows no disclaimer", test_view_health_insurance_no_disclaimer);
    CU_add_test(suite, "Emergency Fund deposit updates the balance", test_emergency_fund_deposit);
    CU_add_test(suite, "Emergency Fund deposit persists to disk", test_emergency_fund_deposit_persists);
    CU_add_test(suite, "Emergency Fund withdraw rejects insufficient balance", test_emergency_fund_withdraw_insufficient);
    CU_add_test(suite, "Emergency Fund withdraw succeeds within balance", test_emergency_fund_withdraw_success);
    CU_add_test(suite, "Emergency Fund deposit rejects zero amount", test_emergency_fund_deposit_rejects_zero);
    CU_add_test(suite, "Emergency Fund deposit cancels via -1", test_emergency_fund_deposit_cancel);
    CU_add_test(suite, "recommendation splits a valid amount correctly", test_recommendation_valid_amount);
    CU_add_test(suite, "recommendation cancels via -1", test_recommendation_cancel);
    CU_add_test(suite, "recommendation rejects a zero amount", test_recommendation_rejects_zero);
    CU_add_test(suite, "investment menu rejects an invalid choice", test_invalid_menu_choice);
}
