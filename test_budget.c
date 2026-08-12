#include "CUnit.h"
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"
#include "budget.h"
#include "category_index.h"
#include "common.h"

/* setBudget()/viewBudgets() call saveBudgets(), which writes
   "<currentUser>_budgets.dat" into the working directory - so this
   suite runs from a scratch temp dir. */
static int suite_init(void)
{
    th_enter_tmp_dir();
    th_reset_globals();
    strcpy(currentUser, "buduser");
    return 0;
}

static int suite_clean(void)
{
    th_leave_tmp_dir();
    return 0;
}

static void add_transaction(const char *type, const char *category, float amount)
{
    Transaction *t = &transactions[transactionCount];

    strcpy(t->type, type);
    strcpy(t->category, category);
    t->amount = amount;
    strcpy(t->date, "2026-01-01");
    t->id = ++transactionCount;
}

/* getSpentForCategory() should sum only Expense transactions in
   the requested category, ignoring Income and other categories. */
static void test_getSpentForCategory_sums_expenses_only(void)
{
    th_reset_globals();

    add_transaction("Expense", "Food", 100.0f);
    add_transaction("Expense", "Food", 50.0f);
    add_transaction("Income", "Food", 500.0f);   /* must be ignored */
    add_transaction("Expense", "Travel", 75.0f); /* different category */

    float spent = getSpentForCategory("Food");

    CU_ASSERT_DOUBLE_EQUAL(spent, 150.0, 0.001);
}

/* A category with no matching transactions should sum to 0. */
static void test_getSpentForCategory_no_match_is_zero(void)
{
    th_reset_globals();

    add_transaction("Expense", "Food", 100.0f);

    float spent = getSpentForCategory("Rent");

    CU_ASSERT_DOUBLE_EQUAL(spent, 0.0, 0.001);
}

/* setBudget() now only offers categories that already exist in
   transactions[] - with none logged yet, it should say so and
   read no further input (a category prompt or number would be
   waiting to consume input the test never provides). */
static void test_setBudget_no_categories_yet(void)
{
    th_reset_globals();

    th_capture_stdout_start();
    setBudget();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(budgetCount, 0);
    CU_ASSERT(strstr(out, "No transaction categories available yet") != NULL);
    free(out);
}

/* Picking a valid category by number, for a category with no
   existing budget, should add a new budget entry. */
static void test_setBudget_new_category(void)
{
    th_reset_globals();
    add_transaction("Expense", "Food", 20.0f);

    th_feed_stdin("1\n300\n");
    th_capture_stdout_start();
    setBudget();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(budgetCount, 1);
    CU_ASSERT_STRING_EQUAL(budgets[0].category, "Food");
    CU_ASSERT_DOUBLE_EQUAL(budgets[0].limit, 300.0, 0.001);
    CU_ASSERT(strstr(out, "Budget Set Successfully") != NULL);
    free(out);
}

/* Picking a category that already has a budget should update the
   existing entry in place rather than adding a duplicate one. */
static void test_setBudget_existing_category_updates(void)
{
    th_reset_globals();
    add_transaction("Expense", "Food", 20.0f);
    strcpy(budgets[0].category, "Food");
    budgets[0].limit = 300.0f;
    budgetCount = 1;

    th_feed_stdin("1\n450\n");
    th_capture_stdout_start();
    setBudget();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(budgetCount, 1);
    CU_ASSERT_DOUBLE_EQUAL(budgets[0].limit, 450.0, 0.001);
    CU_ASSERT(strstr(out, "Budget Updated Successfully") != NULL);
    free(out);
}

/* Entering "0" at the category-number prompt should cancel without
   touching budgetCount. */
static void test_setBudget_cancel_via_category_number(void)
{
    th_reset_globals();
    add_transaction("Expense", "Food", 20.0f);

    th_feed_stdin("0\n");
    th_capture_stdout_start();
    setBudget();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(budgetCount, 0);
    CU_ASSERT(strstr(out, "Cancelled") != NULL);
    free(out);
}

/* A category number outside the listed range should be rejected
   with "Invalid Choice" rather than silently accepted or crashing. */
static void test_setBudget_invalid_category_number(void)
{
    th_reset_globals();
    add_transaction("Expense", "Food", 20.0f);

    th_feed_stdin("99\n");
    th_capture_stdout_start();
    setBudget();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(budgetCount, 0);
    CU_ASSERT(strstr(out, "Invalid Choice") != NULL);
    free(out);
}

/* Entering -1 for the limit should cancel after a valid category
   has already been picked, without adding a budget. */
static void test_setBudget_cancel_via_limit(void)
{
    th_reset_globals();
    add_transaction("Expense", "Food", 20.0f);

    th_feed_stdin("1\n-1\n");
    th_capture_stdout_start();
    setBudget();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(budgetCount, 0);
    CU_ASSERT(strstr(out, "Cancelled") != NULL);
    free(out);
}

/* When MAX_BUDGETS is already reached, setBudget() must refuse to
   add another one and must not read/consume any input (not even
   the category list, since it returns before that point). */
static void test_setBudget_limit_reached(void)
{
    th_reset_globals();
    budgetCount = MAX_BUDGETS;

    th_capture_stdout_start();
    setBudget();
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(budgetCount, MAX_BUDGETS);
    CU_ASSERT(strstr(out, "Budget Limit Reached") != NULL);
    free(out);
}

/* setBudget() keeps budgets[] sorted by category on every insert
   (not just appending at the end) - this is what makes the binary
   search in findBudgetIndex() valid. Insert out of alphabetical
   order (by picking categories in a different order than they'd
   sort) and confirm the array ends up sorted regardless. */
static void test_setBudget_maintains_sorted_order(void)
{
    th_reset_globals();
    add_transaction("Expense", "Travel", 10.0f);
    add_transaction("Expense", "Food", 10.0f);
    add_transaction("Expense", "Books", 10.0f);
    /* Existing Categories list will read: 1. Travel  2. Food  3. Books */

    th_feed_stdin("1\n500\n");
    free(th_call_capturing(setBudget));

    th_feed_stdin("2\n100\n");
    free(th_call_capturing(setBudget));

    th_feed_stdin("3\n200\n");
    free(th_call_capturing(setBudget));

    CU_ASSERT_EQUAL(budgetCount, 3);
    CU_ASSERT_STRING_EQUAL(budgets[0].category, "Books");
    CU_ASSERT_STRING_EQUAL(budgets[1].category, "Food");
    CU_ASSERT_STRING_EQUAL(budgets[2].category, "Travel");
}

/* findBudgetIndex() binary search: given a sorted budgets[]
   array, it should locate the first, middle, and last entries
   correctly, and return -1 for a category that isn't present. */
static void test_findBudgetIndex_binary_search(void)
{
    th_reset_globals();

    strcpy(budgets[0].category, "Alpha");
    budgets[0].limit = 10.0f;
    strcpy(budgets[1].category, "Food");
    budgets[1].limit = 20.0f;
    strcpy(budgets[2].category, "Zebra");
    budgets[2].limit = 30.0f;
    budgetCount = 3;

    CU_ASSERT_EQUAL(findBudgetIndex("Alpha"), 0);
    CU_ASSERT_EQUAL(findBudgetIndex("Food"), 1);
    CU_ASSERT_EQUAL(findBudgetIndex("Zebra"), 2);
    CU_ASSERT_EQUAL(findBudgetIndex("Missing"), -1);
}

/* findBudgetIndex() on an empty budgets[] should return -1, not
   read out of bounds or crash. */
static void test_findBudgetIndex_empty(void)
{
    th_reset_globals();

    CU_ASSERT_EQUAL(findBudgetIndex("Anything"), -1);
}

/* sortBudgetsByCategory() should reorder an out-of-order budgets[]
   array alphabetically by category, using qsort(). */
static void test_sortBudgetsByCategory(void)
{
    th_reset_globals();

    strcpy(budgets[0].category, "Zebra");
    budgets[0].limit = 30.0f;
    strcpy(budgets[1].category, "Alpha");
    budgets[1].limit = 10.0f;
    strcpy(budgets[2].category, "Food");
    budgets[2].limit = 20.0f;
    budgetCount = 3;

    sortBudgetsByCategory();

    CU_ASSERT_STRING_EQUAL(budgets[0].category, "Alpha");
    CU_ASSERT_STRING_EQUAL(budgets[1].category, "Food");
    CU_ASSERT_STRING_EQUAL(budgets[2].category, "Zebra");
}

/* viewBudgets() with no budgets set should say so and not print
   a table. */
static void test_viewBudgets_empty(void)
{
    th_reset_globals();

    th_capture_stdout_start();
    viewBudgets();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "No Budgets Set") != NULL);
    free(out);
}

/* viewBudgets() should flag a category that's over its limit as
   [OVER BUDGET] and one safely under it as [OK]. */
static void test_viewBudgets_flags_over_and_ok(void)
{
    th_reset_globals();

    strcpy(budgets[0].category, "Food");
    budgets[0].limit = 100.0f;
    strcpy(budgets[1].category, "Travel");
    budgets[1].limit = 500.0f;
    budgetCount = 2;

    add_transaction("Expense", "Food", 150.0f);   /* over */
    add_transaction("Expense", "Travel", 50.0f);  /* well under */

    th_capture_stdout_start();
    viewBudgets();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "[OVER BUDGET]") != NULL);
    CU_ASSERT(strstr(out, "[OK]") != NULL);
    free(out);
}

/* checkBudgetAlert() should look up the budget for the given
   category via the hash index and forward spent/limit into the
   notification layer, producing an [ALERT] when over budget.
   Since the test seeds budgets[] directly (bypassing setBudget(),
   which is what normally rebuilds the index), it must call
   categoryIndexRebuild() itself first - exactly as setBudget()
   and loadBudgets() do in the real code paths. */
static void test_checkBudgetAlert_triggers_alert_when_over(void)
{
    th_reset_globals();

    strcpy(budgets[0].category, "Food");
    budgets[0].limit = 100.0f;
    budgetCount = 1;
    categoryIndexRebuild();

    add_transaction("Expense", "Food", 120.0f);

    th_capture_stdout_start();
    checkBudgetAlert("Food");
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "[ALERT]") != NULL);
    free(out);
}

/* checkBudgetAlert() for a category with no budget set at all
   should do nothing (no crash, no output). Also confirms the hash
   index correctly reflects an empty budgets[] rather than holding
   stale entries from an earlier test. */
static void test_checkBudgetAlert_no_budget_for_category(void)
{
    th_reset_globals();
    categoryIndexRebuild();

    th_capture_stdout_start();
    checkBudgetAlert("Untracked");
    char *out = th_capture_stdout_end();

    CU_ASSERT_STRING_EQUAL(out, "");
    free(out);
}

/* End-to-end: a budget set through the real setBudget() path
   (which rebuilds the index itself) should be found correctly by
   checkBudgetAlert() afterwards, with no manual index rebuild
   needed - this is the real usage pattern main.c relies on. */
static void test_checkBudgetAlert_after_real_setBudget(void)
{
    th_reset_globals();
    strcpy(currentUser, "buduser");
    add_transaction("Expense", "Food", 10.0f);

    th_feed_stdin("1\n50\n");
    free(th_call_capturing(setBudget));

    add_transaction("Expense", "Food", 80.0f);

    th_capture_stdout_start();
    checkBudgetAlert("Food");
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "[ALERT]") != NULL);
    free(out);
}

void add_budget_suite(void)
{
    CU_pSuite suite = CU_add_suite("budget", suite_init, suite_clean);

    CU_add_test(suite, "getSpentForCategory sums expenses only", test_getSpentForCategory_sums_expenses_only);
    CU_add_test(suite, "getSpentForCategory is 0 with no match", test_getSpentForCategory_no_match_is_zero);
    CU_add_test(suite, "setBudget reports no categories yet", test_setBudget_no_categories_yet);
    CU_add_test(suite, "setBudget adds a new category by number", test_setBudget_new_category);
    CU_add_test(suite, "setBudget updates an existing category by number", test_setBudget_existing_category_updates);
    CU_add_test(suite, "setBudget cancels via category number '0'", test_setBudget_cancel_via_category_number);
    CU_add_test(suite, "setBudget rejects an out-of-range category number", test_setBudget_invalid_category_number);
    CU_add_test(suite, "setBudget cancels via limit -1", test_setBudget_cancel_via_limit);
    CU_add_test(suite, "setBudget refuses past MAX_BUDGETS", test_setBudget_limit_reached);
    CU_add_test(suite, "setBudget keeps budgets[] sorted by category", test_setBudget_maintains_sorted_order);
    CU_add_test(suite, "findBudgetIndex binary search locates entries", test_findBudgetIndex_binary_search);
    CU_add_test(suite, "findBudgetIndex on empty budgets[] is -1", test_findBudgetIndex_empty);
    CU_add_test(suite, "sortBudgetsByCategory sorts via qsort", test_sortBudgetsByCategory);
    CU_add_test(suite, "viewBudgets reports empty state", test_viewBudgets_empty);
    CU_add_test(suite, "viewBudgets flags OVER BUDGET and OK", test_viewBudgets_flags_over_and_ok);
    CU_add_test(suite, "checkBudgetAlert alerts when over budget", test_checkBudgetAlert_triggers_alert_when_over);
    CU_add_test(suite, "checkBudgetAlert is silent with no budget", test_checkBudgetAlert_no_budget_for_category);
    CU_add_test(suite, "checkBudgetAlert works after real setBudget", test_checkBudgetAlert_after_real_setBudget);
}
