#include "CUnit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"
#include "logger.h"

static int suite_init(void)
{
    return 0;
}

static int suite_clean(void)
{
    return 0;
}

/* Reads a whole file into a malloc'd, NUL-terminated buffer.
   Caller must free() the result. Returns NULL if the file
   can't be opened. */
static char *readWholeFile(const char *path)
{
    FILE *fp = fopen(path, "rb");
    char *buf;
    long size;

    if(fp == NULL)
        return NULL;

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buf = malloc((size_t)size + 1u);
    if(buf == NULL)
    {
        fclose(fp);
        return NULL;
    }

    if(fread(buf, 1, (size_t)size, fp) != (size_t)size)
    {
        free(buf);
        fclose(fp);
        return NULL;
    }
    buf[size] = '\0';

    fclose(fp);
    return buf;
}

/* logInit() should succeed and produce a file containing a
   "logging started" line. */
static void test_init_creates_file_with_startup_line(void)
{
    th_enter_tmp_dir();

    CU_ASSERT(logInit("test.log") != 0);
    logClose();

    char *content = readWholeFile("test.log");
    CU_ASSERT_PTR_NOT_NULL(content);
    if(content != NULL)
    {
        CU_ASSERT(strstr(content, "logging started") != NULL);
        CU_ASSERT(strstr(content, "[INFO ]") != NULL);
        free(content);
    }

    th_leave_tmp_dir();
}

/* An INFO-level message logged at the default (production)
   level should appear in the file. */
static void test_info_message_is_written(void)
{
    th_enter_tmp_dir();

    logInit("test.log");
    LOG_INFO_MSG("hello from a test");
    logClose();

    char *content = readWholeFile("test.log");
    CU_ASSERT_PTR_NOT_NULL(content);
    if(content != NULL)
    {
        CU_ASSERT(strstr(content, "hello from a test") != NULL);
        CU_ASSERT(strstr(content, "[INFO ]") != NULL);
        free(content);
    }

    th_leave_tmp_dir();
}

/* At the default production level (LOG_INFO), a DEBUG-level
   message should be filtered out and never reach the file. */
static void test_debug_filtered_at_production_level(void)
{
    th_enter_tmp_dir();

    logInit("test.log");
    logSetLevel(LOG_INFO);
    LOG_DEBUG_MSG("should not appear");
    logClose();

    char *content = readWholeFile("test.log");
    CU_ASSERT_PTR_NOT_NULL(content);
    if(content != NULL)
    {
        CU_ASSERT(strstr(content, "should not appear") == NULL);
        free(content);
    }

    th_leave_tmp_dir();
}

/* Explicitly lowering the minimum level to LOG_DEBUG should let
   DEBUG-level messages through - this is the "debug vs production"
   switch exercised at its lowest level. */
static void test_debug_visible_after_lowering_level(void)
{
    th_enter_tmp_dir();

    logInit("test.log");
    logSetLevel(LOG_DEBUG);
    LOG_DEBUG_MSG("now visible");
    logClose();

    char *content = readWholeFile("test.log");
    CU_ASSERT_PTR_NOT_NULL(content);
    if(content != NULL)
    {
        CU_ASSERT(strstr(content, "now visible") != NULL);
        CU_ASSERT(strstr(content, "[DEBUG]") != NULL);
        free(content);
    }

    th_leave_tmp_dir();
}

/* An ERROR line should record the level, the calling function
   name, and the message - this is the "error tracing" property:
   given only app.log, you can find exactly where a problem was
   raised without a debugger attached. */
static void test_error_message_includes_function_name(void)
{
    th_enter_tmp_dir();

    logInit("test.log");
    LOG_ERROR_MSG("something went wrong");
    logClose();

    char *content = readWholeFile("test.log");
    CU_ASSERT_PTR_NOT_NULL(content);
    if(content != NULL)
    {
        CU_ASSERT(strstr(content, "[ERROR]") != NULL);
        CU_ASSERT(strstr(content, "something went wrong") != NULL);
        /* every LOG_*_MSG call site in this test file is inside
           this function, so its name must appear in the line */
        CU_ASSERT(strstr(content, "test_error_message_includes_function_name") != NULL);
        free(content);
    }

    th_leave_tmp_dir();
}

/* Every line logInit()/logMessage()/logClose() write should start
   with a "[YYYY-MM-DD HH:MM:SS]" timestamp. */
static void test_timestamp_format(void)
{
    th_enter_tmp_dir();

    logInit("test.log");
    logClose();

    char *content = readWholeFile("test.log");
    CU_ASSERT_PTR_NOT_NULL(content);
    if(content != NULL)
    {
        int year, month, day, hour, minute, second;
        int parsed = sscanf(content, "[%d-%d-%d %d:%d:%d]",
                             &year, &month, &day, &hour, &minute, &second);

        CU_ASSERT_EQUAL(parsed, 6);
        CU_ASSERT(year >= 2026);
        CU_ASSERT(month >= 1 && month <= 12);
        CU_ASSERT(day >= 1 && day <= 31);

        free(content);
    }

    th_leave_tmp_dir();
}

/* Logging enough lines to cross the rotation threshold should
   produce a "<path>.old" backup file, and the active log file
   should still be writable afterward (rotation doesn't break
   further logging). */
static void test_rotation_creates_backup_file(void)
{
    int i;

    th_enter_tmp_dir();

    logInit("test.log");
    logSetLevel(LOG_DEBUG);

    /* Each line is well over 60 bytes; a few hundred lines is
       comfortably past the module's 50KB rotation threshold
       without depending on its exact value. */
    for(i = 0; i < 1000; i++)
        LOG_DEBUG_MSG("padding line to force the log file past the rotation size threshold");

    logClose();

    FILE *oldFp = fopen("test.log.old", "rb");
    CU_ASSERT_PTR_NOT_NULL(oldFp);
    if(oldFp != NULL)
        fclose(oldFp);

    /* logging should still work after a rotation happened */
    logInit("test.log");
    LOG_INFO_MSG("still logging after rotation");
    logClose();

    char *content = readWholeFile("test.log");
    CU_ASSERT_PTR_NOT_NULL(content);
    if(content != NULL)
    {
        CU_ASSERT(strstr(content, "still logging after rotation") != NULL);
        free(content);
    }

    th_leave_tmp_dir();
}

/* logInit() should fail gracefully (return 0, not crash) when
   given a path that can't possibly be created, e.g. inside a
   directory that doesn't exist. */
static void test_init_fails_gracefully_on_bad_path(void)
{
    th_enter_tmp_dir();

    CU_ASSERT_EQUAL(logInit("no/such/directory/test.log"), 0);

    /* logging after a failed init must not crash - it should be
       a silent no-op since logFile stays NULL */
    LOG_ERROR_MSG("this should not crash even though init failed");
    logClose();

    th_leave_tmp_dir();
}

void add_logger_suite(void)
{
    CU_pSuite suite = CU_add_suite("logger", suite_init, suite_clean);

    CU_add_test(suite, "logInit creates a file with a startup line", test_init_creates_file_with_startup_line);
    CU_add_test(suite, "an INFO message is written to the file", test_info_message_is_written);
    CU_add_test(suite, "DEBUG is filtered out at the production level", test_debug_filtered_at_production_level);
    CU_add_test(suite, "DEBUG becomes visible after logSetLevel(LOG_DEBUG)", test_debug_visible_after_lowering_level);
    CU_add_test(suite, "an ERROR line includes the calling function name", test_error_message_includes_function_name);
    CU_add_test(suite, "every line starts with a YYYY-MM-DD HH:MM:SS timestamp", test_timestamp_format);
    CU_add_test(suite, "crossing the size threshold rotates to a .old backup", test_rotation_creates_backup_file);
    CU_add_test(suite, "logInit fails gracefully on an unwritable path", test_init_fails_gracefully_on_bad_path);
}
