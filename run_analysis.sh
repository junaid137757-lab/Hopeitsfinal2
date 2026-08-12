#!/bin/bash
# Runs static and dynamic analysis. Requires cppcheck and valgrind
# to be installed. Run build_tests.sh's compile step first (or this
# script rebuilds run_tests itself before the dynamic checks).
set -e

APP_NO_MAIN="globals.c utility.c notification.c authentication.c persistence.c \
    income.c expense.c transaction.c budget.c savings_goal.c dashboard.c \
    reports.c recommendation.c investment.c category_index.c activity_log.c \
    logger.c autosave.c sha256.c"

TEST_SRC="test_runner.c test_helpers.c test_utility.c test_notification.c \
    test_budget.c test_persistence.c test_authentication.c test_income.c \
    test_expense.c test_transaction.c test_savings_goal.c test_reports.c \
    test_recommendation.c test_dashboard.c test_investment.c \
    test_category_index.c test_activity_log.c test_logger.c test_autosave.c"

echo "=== cppcheck (static analysis) ==="
cppcheck --enable=warning,style,performance,portability \
    --inconclusive --std=c11 --language=c \
    --suppress=missingIncludeSystem \
    main.c $APP_NO_MAIN 2>&1 | tee cppcheckreport.txt

echo
echo "=== MISRA-C:2012 addon check ==="
cppcheck --addon=misra --enable=all --inconclusive --std=c11 \
    --suppress=missingIncludeSystem \
    main.c $APP_NO_MAIN 2>&1 | tee misracreport.txt

echo
echo "=== building test binary for valgrind/helgrind ==="
mkdir -p data
gcc $APP_NO_MAIN $TEST_SRC -Wall -Wextra -pthread -g -lcunit -o run_tests

echo
echo "=== valgrind (memcheck) ==="
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
    --error-exitcode=1 ./run_tests 2>&1 | tee valgrindreport.txt

echo
echo "=== helgrind (data race detector) ==="
valgrind --tool=helgrind --error-exitcode=1 ./run_tests 2>&1 | tee helgrindreport.txt

echo
echo "All reports written: cppcheckreport.txt misracreport.txt valgrindreport.txt helgrindreport.txt"
