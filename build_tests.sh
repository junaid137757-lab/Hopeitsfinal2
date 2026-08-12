#!/bin/bash
# Builds and runs the CUnit test suite. Requires libcunit1-dev /
# cunit-devel to be installed (provides -lcunit and CUnit/CUnit.h).
# Excludes main.c and benchmark.c - test_runner.c has its own main().
set -e
mkdir -p data
gcc globals.c utility.c notification.c authentication.c persistence.c \
    income.c expense.c transaction.c budget.c savings_goal.c dashboard.c \
    reports.c recommendation.c investment.c category_index.c activity_log.c \
    logger.c autosave.c sha256.c \
    test_runner.c test_helpers.c test_utility.c test_notification.c \
    test_budget.c test_persistence.c test_authentication.c test_income.c \
    test_expense.c test_transaction.c test_savings_goal.c test_reports.c \
    test_recommendation.c test_dashboard.c test_investment.c \
    test_category_index.c test_activity_log.c test_logger.c test_autosave.c \
    -Wall -Wextra -pthread -lcunit -o run_tests
echo "Built ./run_tests - running now:"
./run_tests
