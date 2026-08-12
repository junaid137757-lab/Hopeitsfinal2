\
# ============================================================
# Makefile - Digital Personal Finance Platform (flat layout)
#
# All .c/.h files live in this one directory. Three files each
# have their own main() (main.c, test_runner.c, benchmark.c),
# so they are NEVER compiled together in one binary - each
# target below uses an explicit, mutually-exclusive file list.
#
# Quick start:
#   make            -> build the app only            (./finance)
#   make check       -> tests + cppcheck + MISRA + valgrind + helgrind
#   make all         -> check + optimization report, in one command
#
# Individual pieces:
#   make app         -> build ./finance
#   make test        -> build & run ./run_tests (CUnit)
#   make test-noinstall -> same, but via the bundled CUnit-API shim
#                          (CUnit.h/Basic.h/mycunit_storage.c at repo
#                          root; no libcunit1-dev required)
#   make cppcheck    -> static analysis  -> cppcheckreport.txt
#   make misra       -> MISRA-C:2012     -> misracreport.txt
#   make valgrind    -> memcheck         -> valgrindreport.txt
#   make helgrind    -> race detector    -> helgrindreport.txt
#   make tsan        -> ThreadSanitizer build/run (fast alt to helgrind)
#   make optimize    -> -O0..-O3 asm/binary size report -> optimization_report.md
#   make clean       -> remove all build artifacts and reports
#
# Requirements (install what you don't have):
#   apt-get install libcunit1-dev cppcheck valgrind      # Debian/Ubuntu
#   yum install cunit-devel cppcheck valgrind            # RHEL/CentOS
# ============================================================

CC       := gcc
CFLAGS   := -Wall -Wextra -pthread
CFLAGS_G := -Wall -Wextra -pthread -g
LDLIBS   := -lcunit

APP_SRC := globals.c utility.c notification.c authentication.c persistence.c \
           income.c expense.c transaction.c budget.c savings_goal.c dashboard.c \
           reports.c recommendation.c investment.c category_index.c activity_log.c \
           logger.c autosave.c sha256.c

TEST_SRC := test_runner.c test_helpers.c test_utility.c test_notification.c \
            test_budget.c test_persistence.c test_authentication.c test_income.c \
            test_expense.c test_transaction.c test_savings_goal.c test_reports.c \
            test_recommendation.c test_dashboard.c test_investment.c \
            test_category_index.c test_activity_log.c test_logger.c test_autosave.c

APP_BIN   := finance
TEST_BIN  := run_tests
TSAN_BIN  := run_tests_tsan

.PHONY: all app test check cppcheck misra valgrind helgrind tsan optimize clean help

# Default target: just build the interactive app.
all: check optimize

app: $(APP_BIN)

$(APP_BIN): main.c $(APP_SRC)
	mkdir -p data
	$(CC) main.c $(APP_SRC) $(CFLAGS) -o $(APP_BIN)
	@echo "Built ./$(APP_BIN)"

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(APP_SRC) $(TEST_SRC)
	mkdir -p data
	$(CC) $(APP_SRC) $(TEST_SRC) $(CFLAGS_G) $(LDLIBS) -o $(TEST_BIN)

# Fallback for environments where libcunit1-dev/cunit-devel can't be
# installed (e.g. no network access): links against the header-only
# CUnit-API shim (CUnit.h/Basic.h/mycunit_storage.c, at repo root)
# instead of -lcunit. Same test files, unmodified aside from
# quoting the include - see MISRA_DEVIATIONS.md.
test-noinstall: $(APP_SRC) $(TEST_SRC)
	mkdir -p data
	$(CC) $(APP_SRC) $(TEST_SRC) mycunit_storage.c $(CFLAGS_G) -o $(TEST_BIN)
	./$(TEST_BIN)

# One command for the whole verification pass: build, run tests,
# static analysis (cppcheck + MISRA), then dynamic analysis
# (valgrind memcheck + helgrind) on the same test binary.
check: cppcheck misra valgrind helgrind

cppcheck:
	@echo "=== cppcheck (static analysis) ==="
	cppcheck --enable=warning,style,performance,portability \
	    --inconclusive --std=c11 --language=c \
	    --suppress=missingIncludeSystem \
	    main.c $(APP_SRC) 2>&1 | tee cppcheckreport.txt

misra:
	@echo "=== MISRA-C:2012 addon check ==="
	cppcheck --addon=misra --enable=all --inconclusive --std=c11 \
	    --suppress=missingIncludeSystem \
	    main.c $(APP_SRC) 2>&1 | tee misracreport.txt

valgrind: $(TEST_BIN)
	@echo "=== valgrind (memcheck) ==="
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
	    --error-exitcode=1 ./$(TEST_BIN) 2>&1 | tee valgrindreport.txt

helgrind: $(TEST_BIN)
	@echo "=== helgrind (data race detector) ==="
	valgrind --tool=helgrind --error-exitcode=1 ./$(TEST_BIN) 2>&1 | tee helgrindreport.txt

# ThreadSanitizer: needs its own build (can't mix -fsanitize=thread
# into the plain valgrind test binary), so it's a separate target.
tsan:
	mkdir -p data
	$(CC) $(APP_SRC) $(TEST_SRC) -fsanitize=thread -pthread -g $(LDLIBS) -o $(TSAN_BIN)
	./$(TSAN_BIN)

optimize:
	bash optimization_report.sh

clean:
	rm -f $(APP_BIN) $(TEST_BIN) $(TSAN_BIN) finance_O0 finance_O1 finance_O2 finance_O3
	rm -f cppcheckreport.txt misracreport.txt valgrindreport.txt helgrindreport.txt
	rm -f optimization_report.md
	rm -rf asm

help:
	@sed -n '1,30p' Makefile
