#include "CUnit.h"
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"
#include "savings_goal.h"
#include "common.h"

/* setSavingsGoal()/contributeToGoal() call saveGoals(). */
static int suite_init(void)
{
    th_enter_tmp_dir();
    th_reset_globals();
    strcpy(currentUser, "goaluser");
    return 0;
}

static int suite_clean(void)
{
    th_leave_tmp_dir();
    return 0;
}

/* A valid name and target amount should add a new goal starting
   at 0 saved. */
static void test_setSavingsGoal_valid(void)
{
    th_reset_globals();
    strcpy(currentUser, "goaluser");

    th_feed_stdin("Emergency Fund\n10000\n");
    th_capture_stdout_start();
    setSavingsGoal();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(goalCount, 1);
    CU_ASSERT_STRING_EQUAL(goals[0].name, "Emergency Fund");
    CU_ASSERT_DOUBLE_EQUAL(goals[0].targetAmount, 10000.0, 0.001);
    CU_ASSERT_DOUBLE_EQUAL(goals[0].savedAmount, 0.0, 0.001);
    CU_ASSERT(strstr(out, "Savings Goal Set Successfully") != NULL);
    free(out);
}

/* Entering "0" for the name should cancel without adding a goal. */
static void test_setSavingsGoal_cancel(void)
{
    th_reset_globals();
    strcpy(currentUser, "goaluser");

    th_feed_stdin("0\n");
    th_capture_stdout_start();
    setSavingsGoal();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(goalCount, 0);
    CU_ASSERT(strstr(out, "Cancelled") != NULL);
    free(out);
}

/* When MAX_GOALS is already reached, setSavingsGoal() must refuse
   to add another one. */
static void test_setSavingsGoal_limit_reached(void)
{
    th_reset_globals();
    strcpy(currentUser, "goaluser");
    goalCount = MAX_GOALS;

    th_capture_stdout_start();
    setSavingsGoal();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(goalCount, MAX_GOALS);
    CU_ASSERT(strstr(out, "Goal Limit Reached") != NULL);
    free(out);
}

/* Contributing an amount below the target should just increase
   savedAmount and report success, without a congratulations
   message. */
static void test_contributeToGoal_partial(void)
{
    th_reset_globals();
    strcpy(currentUser, "goaluser");
    strcpy(goals[0].name, "Vacation");
    goals[0].targetAmount = 2000.0f;
    goals[0].savedAmount = 500.0f;
    goalCount = 1;

    th_feed_stdin("Vacation\n300\n");
    th_capture_stdout_start();
    contributeToGoal();
    char *out = th_capture_stdout_end();

    CU_ASSERT_DOUBLE_EQUAL(goals[0].savedAmount, 800.0, 0.001);
    CU_ASSERT(strstr(out, "Contribution Added Successfully") != NULL);
    CU_ASSERT(strstr(out, "CONGRATULATIONS") == NULL);
    free(out);
}

/* Contributing enough to reach (or pass) the target should print
   the goal-achieved congratulations message. */
static void test_contributeToGoal_reaches_target(void)
{
    th_reset_globals();
    strcpy(currentUser, "goaluser");
    strcpy(goals[0].name, "Vacation");
    goals[0].targetAmount = 1000.0f;
    goals[0].savedAmount = 900.0f;
    goalCount = 1;

    th_feed_stdin("Vacation\n150\n");
    th_capture_stdout_start();
    contributeToGoal();
    char *out = th_capture_stdout_end();

    CU_ASSERT_DOUBLE_EQUAL(goals[0].savedAmount, 1050.0, 0.001);
    CU_ASSERT(strstr(out, "CONGRATULATIONS") != NULL);
    CU_ASSERT(strstr(out, "Vacation") != NULL);
    free(out);
}

/* Contributing to a goal name that doesn't exist should report
   "not found" and leave saved amounts untouched. */
static void test_contributeToGoal_not_found(void)
{
    th_reset_globals();
    strcpy(currentUser, "goaluser");
    strcpy(goals[0].name, "Vacation");
    goals[0].targetAmount = 1000.0f;
    goals[0].savedAmount = 0.0f;
    goalCount = 1;

    th_feed_stdin("Nonexistent\n");
    th_capture_stdout_start();
    contributeToGoal();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Goal Not Found") != NULL);
    CU_ASSERT_DOUBLE_EQUAL(goals[0].savedAmount, 0.0, 0.001);
    free(out);
}

/* With no goals set, contributeToGoal() should print the "no
   goals" message (via viewSavingsGoals()) and return early. */
static void test_contributeToGoal_no_goals(void)
{
    th_reset_globals();
    strcpy(currentUser, "goaluser");

    th_capture_stdout_start();
    contributeToGoal();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "No Savings Goals Set") != NULL);
    free(out);
}

/* viewSavingsGoals() should show correct percentage progress. */
static void test_viewSavingsGoals_shows_progress(void)
{
    th_reset_globals();
    strcpy(goals[0].name, "Car");
    goals[0].targetAmount = 4000.0f;
    goals[0].savedAmount = 1000.0f; /* 25% */
    goalCount = 1;

    th_capture_stdout_start();
    viewSavingsGoals();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "25.00%") != NULL);
    free(out);
}

void add_savings_goal_suite(void)
{
    CU_pSuite suite = CU_add_suite("savings_goal", suite_init, suite_clean);

    CU_add_test(suite, "setSavingsGoal adds a valid goal", test_setSavingsGoal_valid);
    CU_add_test(suite, "setSavingsGoal cancels via name '0'", test_setSavingsGoal_cancel);
    CU_add_test(suite, "setSavingsGoal refuses past MAX_GOALS", test_setSavingsGoal_limit_reached);
    CU_add_test(suite, "contributeToGoal handles a partial contribution", test_contributeToGoal_partial);
    CU_add_test(suite, "contributeToGoal congratulates on reaching target", test_contributeToGoal_reaches_target);
    CU_add_test(suite, "contributeToGoal reports goal not found", test_contributeToGoal_not_found);
    CU_add_test(suite, "contributeToGoal handles no goals set", test_contributeToGoal_no_goals);
    CU_add_test(suite, "viewSavingsGoals shows correct progress %", test_viewSavingsGoals_shows_progress);
}
