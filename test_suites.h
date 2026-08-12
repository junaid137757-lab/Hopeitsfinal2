#ifndef TEST_SUITES_H
#define TEST_SUITES_H

/* Each test_*.c file defines one of these; it builds its CU_pSuite
   and registers all of its CU_add_test() calls. Keeping this list
   here means test_runner.c doesn't need to change shape as suites
   are added - just add the prototype and the call in main(). */

void add_utility_suite(void);
void add_notification_suite(void);
void add_budget_suite(void);
void add_persistence_suite(void);
void add_authentication_suite(void);
void add_income_suite(void);
void add_expense_suite(void);
void add_transaction_suite(void);
void add_savings_goal_suite(void);
void add_reports_suite(void);
void add_recommendation_suite(void);
void add_dashboard_suite(void);
void add_investment_suite(void);
void add_category_index_suite(void);
void add_activity_log_suite(void);
void add_logger_suite(void);
void add_autosave_suite(void);

#endif
