#include "CUnit.h"
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"
#include "notification.h"

static int suite_init(void)
{
    th_reset_globals();
    return 0;
}

static int suite_clean(void)
{
    return 0;
}

/* Spending under 90% of the limit should print nothing at all. */
static void test_notifyBudgetStatus_ok_prints_nothing(void)
{
    th_capture_stdout_start();
    notifyBudgetStatus("Food", 50.0f, 200.0f);
    char *out = th_capture_stdout_end();

    CU_ASSERT_STRING_EQUAL(out, "");
    free(out);
}

/* Spending between 90% and 100% of the limit should print a
   [WARNING], not an [ALERT]. */
static void test_notifyBudgetStatus_near_limit_warns(void)
{
    th_capture_stdout_start();
    notifyBudgetStatus("Food", 180.0f, 200.0f);
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "[WARNING]") != NULL);
    CU_ASSERT(strstr(out, "[ALERT]") == NULL);
    CU_ASSERT(strstr(out, "Food") != NULL);
    free(out);
}

/* Spending over the limit should print an [ALERT]. */
static void test_notifyBudgetStatus_over_limit_alerts(void)
{
    th_capture_stdout_start();
    notifyBudgetStatus("Food", 250.0f, 200.0f);
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "[ALERT]") != NULL);
    CU_ASSERT(strstr(out, "250.00") != NULL);
    CU_ASSERT(strstr(out, "200.00") != NULL);
    free(out);
}

/* A limit of exactly 0 must not trigger a false "near limit"
   warning through a stray 0 >= 0 comparison. */
static void test_notifyBudgetStatus_zero_limit_no_warning(void)
{
    th_capture_stdout_start();
    notifyBudgetStatus("Misc", 0.0f, 0.0f);
    char *out = th_capture_stdout_end();

    CU_ASSERT_STRING_EQUAL(out, "");
    free(out);
}

static void test_notifyGoalAchieved_prints_name(void)
{
    th_capture_stdout_start();
    notifyGoalAchieved("Emergency Fund");
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "CONGRATULATIONS") != NULL);
    CU_ASSERT(strstr(out, "Emergency Fund") != NULL);
    free(out);
}

void add_notification_suite(void)
{
    CU_pSuite suite = CU_add_suite("notification", suite_init, suite_clean);

    CU_add_test(suite, "under 90% prints nothing", test_notifyBudgetStatus_ok_prints_nothing);
    CU_add_test(suite, "90-100% prints WARNING", test_notifyBudgetStatus_near_limit_warns);
    CU_add_test(suite, "over limit prints ALERT", test_notifyBudgetStatus_over_limit_alerts);
    CU_add_test(suite, "zero limit does not warn", test_notifyBudgetStatus_zero_limit_no_warning);
    CU_add_test(suite, "notifyGoalAchieved prints goal name", test_notifyGoalAchieved_prints_name);
}
