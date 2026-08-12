#include "CUnit.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "test_helpers.h"
#include "utility.h"

static int suite_init(void)
{
    th_reset_globals();
    return 0;
}

static int suite_clean(void)
{
    return 0;
}

/* getCurrentDate() should always produce exactly "YYYY-MM-DD"
   (10 characters, digits and dashes in the right places). We
   can't know "today" in advance, so we check the shape instead
   of a fixed value. */
static void test_getCurrentDate_format(void)
{
    char buffer[11] = "";

    getCurrentDate(buffer);

    CU_ASSERT_EQUAL(strlen(buffer), 10);
    CU_ASSERT(isdigit((unsigned char)buffer[0]));
    CU_ASSERT(isdigit((unsigned char)buffer[1]));
    CU_ASSERT(isdigit((unsigned char)buffer[2]));
    CU_ASSERT(isdigit((unsigned char)buffer[3]));
    CU_ASSERT_EQUAL(buffer[4], '-');
    CU_ASSERT(isdigit((unsigned char)buffer[5]));
    CU_ASSERT(isdigit((unsigned char)buffer[6]));
    CU_ASSERT_EQUAL(buffer[7], '-');
    CU_ASSERT(isdigit((unsigned char)buffer[8]));
    CU_ASSERT(isdigit((unsigned char)buffer[9]));
}

/* readLine() should strip the trailing newline and keep spaces. */
static void test_readLine_basic(void)
{
    char buffer[30] = "";

    th_feed_stdin("Grocery Shopping\n");
    readLine(buffer, sizeof(buffer));

    CU_ASSERT_STRING_EQUAL(buffer, "Grocery Shopping");
}

/* An empty line (just Enter) should produce an empty string, not
   garbage or leftover data from a previous call. */
static void test_readLine_empty(void)
{
    char buffer[30] = "prefilled";

    th_feed_stdin("\n");
    readLine(buffer, sizeof(buffer));

    CU_ASSERT_STRING_EQUAL(buffer, "");
}

/* A line longer than the destination buffer must be truncated to
   size-1 characters and must not leave the overflow to corrupt
   whatever readLine() is called next. */
static void test_readLine_overflow_is_truncated(void)
{
    char buffer[6] = "";
    char next[30] = "";

    th_feed_stdin("ABCDEFGHIJ\nSecondLine\n");
    readLine(buffer, sizeof(buffer));
    readLine(next, sizeof(next));

    CU_ASSERT_EQUAL(strlen(buffer), 5);
    CU_ASSERT_STRING_EQUAL(next, "SecondLine");
}

/* readValidInt() on good numeric input should return 1 and store
   the parsed value. */
static void test_readValidInt_valid(void)
{
    int value = -999;

    th_feed_stdin("42\n");
    int ok = readValidInt(&value);

    CU_ASSERT_EQUAL(ok, 1);
    CU_ASSERT_EQUAL(value, 42);
}

/* readValidInt() on non-numeric input should return 0, print an
   error, and must not leave the bad token for the next read. */
static void test_readValidInt_invalid_then_recovers(void)
{
    int value = -999;
    int next = -999;

    th_feed_stdin("abc\n7\n");

    th_capture_stdout_start();
    int ok = readValidInt(&value);
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(ok, 0);
    CU_ASSERT(strstr(out, "Invalid input") != NULL);
    free(out);

    int ok2 = readValidInt(&next);
    CU_ASSERT_EQUAL(ok2, 1);
    CU_ASSERT_EQUAL(next, 7);
}

/* readValidFloat() on good numeric input should return 1 and
   store the parsed value. */
static void test_readValidFloat_valid(void)
{
    float value = -999.0f;

    th_feed_stdin("123.50\n");
    int ok = readValidFloat(&value);

    CU_ASSERT_EQUAL(ok, 1);
    CU_ASSERT_DOUBLE_EQUAL(value, 123.50, 0.001);
}

/* readValidFloat() should reject non-numeric input the same way
   readValidInt() does. */
static void test_readValidFloat_invalid(void)
{
    float value = -999.0f;

    th_capture_stdout_start();
    th_feed_stdin("notanumber\n");
    int ok = readValidFloat(&value);
    char *out = th_capture_stdout_end();

    CU_ASSERT_EQUAL(ok, 0);
    CU_ASSERT(strstr(out, "Invalid input") != NULL);
    free(out);
}

/* caseInsensitiveContains() should find a needle regardless of
   case differences between it and the haystack. */
static void test_caseInsensitiveContains_matches_ignoring_case(void)
{
    CU_ASSERT_EQUAL(caseInsensitiveContains("Stocks Income", "income"), 1);
    CU_ASSERT_EQUAL(caseInsensitiveContains("STOCKS INCOME", "Income"), 1);
    CU_ASSERT_EQUAL(caseInsensitiveContains("stocks income", "INCOME"), 1);
}

/* A needle that genuinely doesn't appear anywhere in the haystack
   should return 0. */
static void test_caseInsensitiveContains_no_match(void)
{
    CU_ASSERT_EQUAL(caseInsensitiveContains("Groceries", "Travel"), 0);
}

/* An empty needle is considered to match anything (mirrors the
   behavior of strstr() with an empty needle), including an empty
   haystack. */
static void test_caseInsensitiveContains_empty_needle(void)
{
    CU_ASSERT_EQUAL(caseInsensitiveContains("Groceries", ""), 1);
    CU_ASSERT_EQUAL(caseInsensitiveContains("", ""), 1);
}

/* An empty haystack with a non-empty needle should never match. */
static void test_caseInsensitiveContains_empty_haystack(void)
{
    CU_ASSERT_EQUAL(caseInsensitiveContains("", "Food"), 0);
}

void add_utility_suite(void)
{
    CU_pSuite suite = CU_add_suite("utility", suite_init, suite_clean);

    CU_add_test(suite, "getCurrentDate produces YYYY-MM-DD", test_getCurrentDate_format);
    CU_add_test(suite, "readLine reads a plain line", test_readLine_basic);
    CU_add_test(suite, "readLine handles an empty line", test_readLine_empty);
    CU_add_test(suite, "readLine truncates overflow safely", test_readLine_overflow_is_truncated);
    CU_add_test(suite, "readValidInt accepts a valid int", test_readValidInt_valid);
    CU_add_test(suite, "readValidInt rejects bad input and recovers", test_readValidInt_invalid_then_recovers);
    CU_add_test(suite, "readValidFloat accepts a valid float", test_readValidFloat_valid);
    CU_add_test(suite, "readValidFloat rejects bad input", test_readValidFloat_invalid);
    CU_add_test(suite, "caseInsensitiveContains matches ignoring case", test_caseInsensitiveContains_matches_ignoring_case);
    CU_add_test(suite, "caseInsensitiveContains returns 0 with no match", test_caseInsensitiveContains_no_match);
    CU_add_test(suite, "caseInsensitiveContains treats empty needle as a match", test_caseInsensitiveContains_empty_needle);
    CU_add_test(suite, "caseInsensitiveContains returns 0 on empty haystack", test_caseInsensitiveContains_empty_haystack);
}
