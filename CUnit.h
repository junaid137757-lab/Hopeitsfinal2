#ifndef MYCUNIT_CUNIT_H
#define MYCUNIT_CUNIT_H

/* =====================================================
   Minimal CUnit-API-compatible shim.

   The real libcunit1-dev/cunit-devel package could not be
   installed in this build sandbox (no network access - see
   MISRA_DEVIATIONS.md / README_BUILD.md for the full
   explanation). This shim implements exactly the subset of
   the CUnit public API that this project's test suite
   actually calls (verified by grepping every test_*.c file
   for CU_* symbols), so the existing test files compile and
   run completely unmodified against it.

   On a machine with real CUnit installed, simply link against
   -lcunit as usual instead of this shim - nothing in the test
   files needs to change either way.
   ===================================================== */

#include <stdio.h>
#include <string.h>

typedef enum { CUE_SUCCESS = 0, CUE_NOMEMORY = 1 } CU_ErrorCode;

typedef void (*CU_InitializeFunc)(void);
typedef int  (*CU_SuiteInitFunc)(void);
typedef int  (*CU_SuiteCleanupFunc)(void);
typedef void (*CU_TestFunc)(void);

#define MYCUNIT_MAX_SUITES 64
#define MYCUNIT_MAX_TESTS_PER_SUITE 64

typedef struct
{
    const char *name;
    CU_TestFunc func;
} MyCUnitTest;

typedef struct
{
    const char *name;
    CU_SuiteInitFunc init;
    CU_SuiteCleanupFunc clean;
    MyCUnitTest tests[MYCUNIT_MAX_TESTS_PER_SUITE];
    int testCount;
} MyCUnitSuite;

typedef MyCUnitSuite *CU_pSuite;

extern MyCUnitSuite myCUnitSuites[MYCUNIT_MAX_SUITES];
extern int myCUnitSuiteCount;
extern int myCUnitAssertsRun;
extern int myCUnitAssertsFailed;
extern int myCUnitCurrentTestFailed;
extern unsigned int myCUnitTotalTestsFailed;
extern const char *myCUnitCurrentFile;
extern int myCUnitCurrentLine;

static inline int CU_initialize_registry(void)
{
    myCUnitSuiteCount = 0;
    myCUnitAssertsRun = 0;
    myCUnitAssertsFailed = 0;
    return CUE_SUCCESS;
}

static inline int CU_get_error(void)
{
    return CUE_SUCCESS;
}

static inline void CU_cleanup_registry(void)
{
    /* nothing to release - static storage */
}

static inline CU_pSuite CU_add_suite(const char *name, CU_SuiteInitFunc init, CU_SuiteCleanupFunc clean)
{
    CU_pSuite suite = &myCUnitSuites[myCUnitSuiteCount];

    suite->name = name;
    suite->init = init;
    suite->clean = clean;
    suite->testCount = 0;
    myCUnitSuiteCount++;

    return suite;
}

static inline void CU_add_test(CU_pSuite suite, const char *name, CU_TestFunc func)
{
    suite->tests[suite->testCount].name = name;
    suite->tests[suite->testCount].func = func;
    suite->testCount++;
}

static inline void myCUnitRecordAssert(int passed, const char *expr, const char *file, int line)
{
    myCUnitAssertsRun++;

    if(passed == 0)
    {
        myCUnitAssertsFailed++;
        myCUnitCurrentTestFailed = 1;
        (void)fprintf(stderr, "    FAILED assertion: %s (%s:%d)\n", expr, file, line);
    }
}

#define CU_ASSERT(expr) \
    myCUnitRecordAssert((expr) ? 1 : 0, #expr, __FILE__, __LINE__)

#define CU_ASSERT_EQUAL(actual, expected) \
    myCUnitRecordAssert(((actual) == (expected)) ? 1 : 0, #actual " == " #expected, __FILE__, __LINE__)

#define CU_ASSERT_DOUBLE_EQUAL(actual, expected, granularity) \
    myCUnitRecordAssert((((actual) - (expected) <= (granularity)) && \
                          ((expected) - (actual) <= (granularity))) ? 1 : 0, \
                         #actual " ~= " #expected, __FILE__, __LINE__)

#define CU_ASSERT_PTR_NOT_NULL(ptr) \
    myCUnitRecordAssert(((ptr) != NULL) ? 1 : 0, #ptr " != NULL", __FILE__, __LINE__)

#define CU_ASSERT_PTR_NULL(ptr) \
    myCUnitRecordAssert(((ptr) == NULL) ? 1 : 0, #ptr " == NULL", __FILE__, __LINE__)

#define CU_ASSERT_STRING_EQUAL(actual, expected) \
    myCUnitRecordAssert((strcmp((actual), (expected)) == 0) ? 1 : 0, #actual " == " #expected, __FILE__, __LINE__)

#endif
