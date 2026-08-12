#include "CUnit.h"
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"
#include "recommendation.h"
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

static void add_transaction(const char *type, const char *category, float amount)
{
    Transaction *t = &transactions[transactionCount];

    strcpy(t->type, type);
    strcpy(t->category, category);
    t->amount = amount;
    strcpy(t->date, "2026-01-01");
    t->id = ++transactionCount;
}

/* With no transactions logged at all, the engine should invite
   the user to start logging rather than analyze empty data. */
static void test_no_transactions_prompts_to_start(void)
{
    th_reset_globals();

    th_capture_stdout_start();
    generateRecommendations();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Start logging your income and expenses") != NULL);
    free(out);
}

/* Expenses exceeding income should trigger the "over budget /
   review spending" style warning. */
static void test_expense_exceeds_income_warns(void)
{
    th_reset_globals();
    add_transaction("Income", "Salary", 1000.0f);
    add_transaction("Expense", "Rent", 1500.0f);

    th_capture_stdout_start();
    generateRecommendations();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "expenses currently exceed your income") != NULL);
    free(out);
}

/* A savings rate under 20% (but not negative) should trigger the
   "below 20%" nudge. */
static void test_low_savings_rate_warns(void)
{
    th_reset_globals();
    add_transaction("Income", "Salary", 1000.0f);
    add_transaction("Expense", "Rent", 900.0f); /* saves only 10% */

    th_capture_stdout_start();
    generateRecommendations();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "savings rate is below 20%") != NULL);
    free(out);
}

/* A healthy savings rate (>= 20%) should trigger the positive
   "great job" message instead of a warning. */
static void test_healthy_savings_rate_praised(void)
{
    th_reset_globals();
    add_transaction("Income", "Salary", 1000.0f);
    add_transaction("Expense", "Rent", 500.0f); /* saves 50% */

    th_capture_stdout_start();
    generateRecommendations();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "Great job") != NULL);
    free(out);
}

/* A category that's over its budget should be called out by
   name, even when the overall savings rate is healthy. */
static void test_over_budget_category_flagged(void)
{
    th_reset_globals();
    add_transaction("Income", "Salary", 5000.0f);
    add_transaction("Expense", "Food", 500.0f);

    strcpy(budgets[0].category, "Food");
    budgets[0].limit = 100.0f; /* 500 spent > 100 limit */
    budgetCount = 1;

    th_capture_stdout_start();
    generateRecommendations();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "over budget in 'Food'") != NULL);
    free(out);
}

/* A goal funded less than halfway should be flagged by name. */
static void test_underfunded_goal_flagged(void)
{
    th_reset_globals();
    add_transaction("Income", "Salary", 5000.0f);

    strcpy(goals[0].name, "Car");
    goals[0].targetAmount = 10000.0f;
    goals[0].savedAmount = 1000.0f; /* 10% funded */
    goalCount = 1;

    th_capture_stdout_start();
    generateRecommendations();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "goal 'Car' is less than halfway funded") != NULL);
    free(out);
}

/* With no goal whose name mentions "emergency" (any case), the
   engine should suggest creating an emergency fund. */
static void test_missing_emergency_fund_flagged(void)
{
    th_reset_globals();
    add_transaction("Income", "Salary", 5000.0f);

    strcpy(goals[0].name, "Car");
    goals[0].targetAmount = 10000.0f;
    goals[0].savedAmount = 10000.0f;
    goalCount = 1;

    th_capture_stdout_start();
    generateRecommendations();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "don't have an emergency fund goal") != NULL);
    free(out);
}

/* Once an "Emergency ..." goal exists, that specific nudge should
   no longer appear. */
static void test_emergency_fund_present_not_flagged(void)
{
    th_reset_globals();
    add_transaction("Income", "Salary", 5000.0f);

    strcpy(goals[0].name, "Emergency Fund");
    goals[0].targetAmount = 10000.0f;
    goals[0].savedAmount = 10000.0f; /* fully funded, so this branch alone won't fire either */
    goalCount = 1;

    th_capture_stdout_start();
    generateRecommendations();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "don't have an emergency fund goal") == NULL);
    free(out);
}

void add_recommendation_suite(void)
{
    CU_pSuite suite = CU_add_suite("recommendation", suite_init, suite_clean);

    CU_add_test(suite, "no transactions prompts to start logging", test_no_transactions_prompts_to_start);
    CU_add_test(suite, "expense exceeding income warns", test_expense_exceeds_income_warns);
    CU_add_test(suite, "savings rate under 20% warns", test_low_savings_rate_warns);
    CU_add_test(suite, "healthy savings rate is praised", test_healthy_savings_rate_praised);
    CU_add_test(suite, "over-budget category is named", test_over_budget_category_flagged);
    CU_add_test(suite, "underfunded goal is named", test_underfunded_goal_flagged);
    CU_add_test(suite, "missing emergency fund is flagged", test_missing_emergency_fund_flagged);
    CU_add_test(suite, "existing emergency fund suppresses the nudge", test_emergency_fund_present_not_flagged);
}
