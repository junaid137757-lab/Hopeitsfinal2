#include "CUnit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"
#include "activity_log.h"

static int suite_init(void)
{
    resetActivityLog();
    return 0;
}

static int suite_clean(void)
{
    return 0;
}

/* An empty log should say so, not print a blank section. */
static void test_empty_log(void)
{
    resetActivityLog();

    th_capture_stdout_start();
    printActivityLog();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "No Activity Logged Yet") != NULL);
    free(out);
}

/* Entries should print most-recent-first - this is the whole point
   of prepending each new node to the head of the in-use list. */
static void test_entries_print_most_recent_first(void)
{
    resetActivityLog();

    logActivity("first");
    logActivity("second");
    logActivity("third");

    th_capture_stdout_start();
    printActivityLog();
    char *out = th_capture_stdout_end();

    const char *posThird = strstr(out, "- third");
    const char *posSecond = strstr(out, "- second");
    const char *posFirst = strstr(out, "- first");

    CU_ASSERT_PTR_NOT_NULL(posThird);
    CU_ASSERT_PTR_NOT_NULL(posSecond);
    CU_ASSERT_PTR_NOT_NULL(posFirst);
    CU_ASSERT(posThird < posSecond);
    CU_ASSERT(posSecond < posFirst);

    free(out);
}

/* Filling the log to exactly its capacity should keep every entry -
   nothing should be dropped early. */
static void test_fill_to_capacity_keeps_everything(void)
{
    int i;
    char msg[32];

    resetActivityLog();

    for(i = 0; i < ACTIVITY_LOG_CAPACITY; i++)
    {
        snprintf(msg, sizeof(msg), "entry-%d", i);
        logActivity(msg);
    }

    th_capture_stdout_start();
    printActivityLog();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "entry-0") != NULL);
    CU_ASSERT(strstr(out, "entry-19") != NULL || ACTIVITY_LOG_CAPACITY != 20);

    free(out);
}

/* Adding one entry past capacity should reclaim the OLDEST entry
   (the free-list-exhausted path in logActivity()), so the very
   first message ever logged should no longer appear, while the
   most recent ACTIVITY_LOG_CAPACITY entries should all still be
   present. */
static void test_exceeding_capacity_drops_oldest(void)
{
    int i;
    char msg[32];

    resetActivityLog();

    for(i = 0; i < ACTIVITY_LOG_CAPACITY + 1; i++)
    {
        snprintf(msg, sizeof(msg), "entry-%d", i);
        logActivity(msg);
    }

    th_capture_stdout_start();
    printActivityLog();
    char *out = th_capture_stdout_end();

    /* entry-0 was the oldest and should have been reclaimed */
    CU_ASSERT(strstr(out, "entry-0\n") == NULL);

    /* the newest entry and the previous oldest survivor should
       both still be present */
    char newest[32];
    char oldestSurvivor[32];

    snprintf(newest, sizeof(newest), "entry-%d", ACTIVITY_LOG_CAPACITY);
    snprintf(oldestSurvivor, sizeof(oldestSurvivor), "entry-1");

    CU_ASSERT(strstr(out, newest) != NULL);
    CU_ASSERT(strstr(out, oldestSurvivor) != NULL);

    free(out);
}

/* A message longer than the internal buffer should be truncated
   safely, not overflow or crash. */
static void test_long_message_is_truncated_safely(void)
{
    char longMsg[300];
    int i;

    resetActivityLog();

    for(i = 0; i < 299; i++)
        longMsg[i] = 'x';
    longMsg[299] = '\0';

    logActivity(longMsg);

    th_capture_stdout_start();
    printActivityLog();
    char *out = th_capture_stdout_end();

    /* Should not crash, and should print something starting with
       a run of 'x' characters. */
    CU_ASSERT(strstr(out, "xxxxxxxxxx") != NULL);
    free(out);
}

/* resetActivityLog() should bring the log back to a clean empty
   state, discarding anything logged before it. */
static void test_reset_clears_log(void)
{
    resetActivityLog();
    logActivity("should be gone");

    resetActivityLog();

    th_capture_stdout_start();
    printActivityLog();
    char *out = th_capture_stdout_end();

    CU_ASSERT(strstr(out, "No Activity Logged Yet") != NULL);
    CU_ASSERT(strstr(out, "should be gone") == NULL);
    free(out);
}

void add_activity_log_suite(void)
{
    CU_pSuite suite = CU_add_suite("activity_log", suite_init, suite_clean);

    CU_add_test(suite, "empty log reports no activity", test_empty_log);
    CU_add_test(suite, "entries print most-recent-first", test_entries_print_most_recent_first);
    CU_add_test(suite, "filling to capacity keeps everything", test_fill_to_capacity_keeps_everything);
    CU_add_test(suite, "exceeding capacity drops the oldest entry", test_exceeding_capacity_drops_oldest);
    CU_add_test(suite, "an overlong message is truncated safely", test_long_message_is_truncated_safely);
    CU_add_test(suite, "resetActivityLog clears the log", test_reset_clears_log);
}
