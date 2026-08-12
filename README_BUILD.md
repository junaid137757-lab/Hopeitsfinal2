# Digital Personal Finance Platform - build & verify

All .c/.h files sit in this one folder (no src/include/tests
subfolders) - matches how this project is laid out on the
deployment box.

## IMPORTANT: three files each have their own main()

- main.c        -> the real interactive app
- test_runner.c -> the CUnit test suite entry point
- benchmark.c   -> a standalone performance/capacity tool

Never compile all of them together (`gcc *.c` will fail with a
"multiple definition of main" linker error). Use the scripts below,
which each use an explicit file list.

## Build & run the app

    bash build_app.sh
    ./finance

## Build & run the CUnit test suite

Requires libcunit1-dev (Debian/Ubuntu) or cunit-devel (RHEL/CentOS):

    yum install cunit-devel      # or: apt-get install libcunit1-dev
    bash build_tests.sh

## Static & dynamic analysis (cppcheck, MISRA, valgrind, helgrind)

Requires cppcheck and valgrind:

    yum install cppcheck valgrind
    bash run_analysis.sh

Writes cppcheckreport.txt, misracreport.txt, valgrindreport.txt,
helgrindreport.txt - check each for "0 errors" / no MISRA violations.

## ThreadSanitizer (fast alternative/second opinion to helgrind)

No extra install needed, just gcc:

    gcc globals.c utility.c notification.c authentication.c persistence.c \
        income.c expense.c transaction.c budget.c savings_goal.c dashboard.c \
        reports.c recommendation.c investment.c category_index.c activity_log.c \
        logger.c autosave.c test_runner.c test_helpers.c test_utility.c \
        test_notification.c test_budget.c test_persistence.c test_authentication.c \
        test_income.c test_expense.c test_transaction.c test_savings_goal.c \
        test_reports.c test_recommendation.c test_dashboard.c test_investment.c \
        test_category_index.c test_activity_log.c test_logger.c test_autosave.c \
        -fsanitize=thread -pthread -g -lcunit -o run_tests_tsan
    ./run_tests_tsan

## Optimization report (-O0/-O1/-O2/-O3 assembly size comparison)

    bash optimization_report.sh

Writes optimization_report.md and asm/O0..O3/*.s.

## Logging & timestamps

Every run writes data/app.log - timestamped, leveled (DEBUG/INFO/WARN/
ERROR), auto-rotating past ~50KB, thread-safe (both the main thread
and the background autosave thread write to it). Check it with:

    cat data/app.log
    tail -f data/app.log   # while the app is running, in a 2nd terminal

## Fixes included in this build (verified in this exact flat layout)

- Transaction ID collision: income.c/expense.c now call
  nextTransactionId() instead of transactionCount + 1, and
  deleteTransaction()/loadTransactions() renumber ids to stay
  compact - no more duplicate or gapped ids.
- Cross-user activity log leak: main.c calls resetActivityLog() on
  every login, so option 19 never shows a previous user's history.
- Thread-safe logger + background autosave thread guarded by
  dataMutex, verified race-free under both ThreadSanitizer and
  (if you run it) real Helgrind.
