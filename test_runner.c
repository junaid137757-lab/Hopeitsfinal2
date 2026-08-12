#include <stdio.h>
#include <stdlib.h>

#include "CUnit.h"
#include "Basic.h"

#include "test_suites.h"

int main(void)
{
    if(CU_initialize_registry() != CUE_SUCCESS)
        return CU_get_error();

    add_utility_suite();
    add_notification_suite();
    add_budget_suite();
    add_persistence_suite();
    add_authentication_suite();
    add_income_suite();
    add_expense_suite();
    add_transaction_suite();
    add_savings_goal_suite();
    add_reports_suite();
    add_recommendation_suite();
    add_dashboard_suite();
    add_investment_suite();
    add_category_index_suite();
    add_activity_log_suite();
    add_logger_suite();
    add_autosave_suite();

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    unsigned int failed = CU_get_number_of_tests_failed();

    CU_cleanup_registry();

    return failed == 0 ? 0 : 1;
}
