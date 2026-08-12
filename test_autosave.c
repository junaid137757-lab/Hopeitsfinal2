#define _DEFAULT_SOURCE
#include "CUnit.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "test_helpers.h"
#include "autosave.h"
#include "common.h"
#include "persistence.h"

static int suite_init(void)
{
    return 0;
}

static int suite_clean(void)
{
    return 0;
}

/* Resets all the shared in-memory state this suite touches, so
   each test starts from a clean slate regardless of test order. */
static void resetSharedState(void)
{
    transactionCount = 0;
    budgetCount = 0;
    goalCount = 0;
    emergencyFundBalance = 0;
    strcpy(currentUser, "autosave_test_user");
}

/* The thread should start and stop cleanly, and stopping should
   actually block until the thread has exited (pthread_join) -
   if it returned early, calling it twice or starting again right
   after would be a real risk of a race/crash rather than a clean
   no-op. */
static void test_start_and_stop_is_clean(void)
{
    th_enter_tmp_dir();
    resetSharedState();

    CU_ASSERT_EQUAL(startAutosaveThread(), 1);
    stopAutosaveThread();

    /* stopping again with nothing running must be a safe no-op */
    stopAutosaveThread();

    th_leave_tmp_dir();
}

/* Starting, stopping, and starting again should work - this is
   exactly the login -> logout -> login pattern main.c drives it
   through in a real session. */
static void test_restart_after_stop(void)
{
    th_enter_tmp_dir();
    resetSharedState();

    CU_ASSERT_EQUAL(startAutosaveThread(), 1);
    stopAutosaveThread();

    CU_ASSERT_EQUAL(startAutosaveThread(), 1);
    stopAutosaveThread();

    th_leave_tmp_dir();
}

/* With the interval shortened, actually wait for a real autosave
   cycle to happen and confirm it wrote the data to disk - this is
   the actual behavior the thread exists for, not just that it can
   start/stop without crashing. */
static void test_autosave_writes_data_to_disk(void)
{
    char filename[80];
    FILE *fp;

    th_enter_tmp_dir();
    resetSharedState();

    autosaveSetIntervalSecondsForTesting(1);

    transactions[0].id = 1;
    strcpy(transactions[0].type, "Income");
    strcpy(transactions[0].category, "Salary");
    transactions[0].amount = 1234.0f;
    strcpy(transactions[0].date, "2026-08-10");
    transactionCount = 1;

    snprintf(filename, sizeof(filename), "data/%s_transactions.dat", currentUser);

    /* nothing saved yet - the file shouldn't exist before the
       thread has had a chance to run even once */
    fp = fopen(filename, "rb");
    CU_ASSERT_PTR_NULL(fp);
    if(fp != NULL)
        fclose(fp);

    CU_ASSERT_EQUAL(startAutosaveThread(), 1);

    /* interval is 1s; give it up to 3s to complete one cycle
       before concluding it didn't run */
    sleep(3);

    stopAutosaveThread();

    fp = fopen(filename, "rb");
    CU_ASSERT_PTR_NOT_NULL(fp);
    if(fp != NULL)
        fclose(fp);

    th_leave_tmp_dir();
}

/* Mutating the shared arrays from the "main thread" (this test)
   while the autosave thread is actively running and saving in the
   background should not crash or corrupt anything - dataMutex is
   what's supposed to make this safe. This won't catch a subtle
   race by itself (that's what the tsan/helgrind Makefile targets
   are for), but it does exercise the exact interleaving pattern a
   real session produces. */
static void test_concurrent_mutation_while_running(void)
{
    int i;

    th_enter_tmp_dir();
    resetSharedState();

    autosaveSetIntervalSecondsForTesting(1);

    CU_ASSERT_EQUAL(startAutosaveThread(), 1);

    for(i = 0; i < 20 && i < MAX; i++)
    {
        pthread_mutex_lock(&dataMutex);
        transactions[transactionCount].id = i + 1;
        strcpy(transactions[transactionCount].type, "Expense");
        strcpy(transactions[transactionCount].category, "Test");
        transactions[transactionCount].amount = (float)i;
        strcpy(transactions[transactionCount].date, "2026-08-10");
        transactionCount++;
        pthread_mutex_unlock(&dataMutex);

        usleep(50000); /* 50ms - give the autosave thread room to interleave */
    }

    stopAutosaveThread();

    CU_ASSERT_EQUAL(transactionCount, 20);

    th_leave_tmp_dir();
}

void add_autosave_suite(void)
{
    CU_pSuite suite = CU_add_suite("autosave", suite_init, suite_clean);

    CU_add_test(suite, "the thread starts and stops cleanly", test_start_and_stop_is_clean);
    CU_add_test(suite, "the thread can be restarted after stopping", test_restart_after_stop);
    CU_add_test(suite, "a real autosave cycle writes data to disk", test_autosave_writes_data_to_disk);
    CU_add_test(suite, "concurrent mutation while the thread runs is safe", test_concurrent_mutation_while_running);
}
