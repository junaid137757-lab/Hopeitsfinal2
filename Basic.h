#ifndef MYCUNIT_BASIC_H
#define MYCUNIT_BASIC_H

#include "CUnit.h"

typedef enum { CU_BRM_NORMAL = 0, CU_BRM_SILENT = 1, CU_BRM_VERBOSE = 2 } CU_BasicRunMode;

static CU_BasicRunMode myCUnitRunMode = CU_BRM_NORMAL;

static inline void CU_basic_set_mode(CU_BasicRunMode mode)
{
    myCUnitRunMode = mode;
}

static inline void CU_basic_run_tests(void)
{
    int s;
    int totalTests = 0;
    int totalFailedTests = 0;

    for(s = 0; s < myCUnitSuiteCount; s++)
    {
        MyCUnitSuite *suite = &myCUnitSuites[s];
        int t;

        if(myCUnitRunMode == CU_BRM_VERBOSE)
        {
            (void)printf("\nSuite: %s\n", suite->name);
        }

        if(suite->init != NULL)
        {
            (void)suite->init();
        }

        for(t = 0; t < suite->testCount; t++)
        {
            myCUnitCurrentTestFailed = 0;
            totalTests++;

            if(myCUnitRunMode == CU_BRM_VERBOSE)
            {
                (void)printf("  Test: %-60s", suite->tests[t].name);
                (void)fflush(stdout);
            }

            suite->tests[t].func();

            if(myCUnitCurrentTestFailed != 0)
            {
                totalFailedTests++;

                if(myCUnitRunMode == CU_BRM_VERBOSE)
                {
                    (void)printf("FAILED\n");
                }
            }
            else
            {
                if(myCUnitRunMode == CU_BRM_VERBOSE)
                {
                    (void)printf("passed\n");
                }
            }
        }

        if(suite->clean != NULL)
        {
            (void)suite->clean();
        }
    }

    (void)printf("\n--------------------------------------------------\n");
    (void)printf("Run Summary: %d tests, %d asserts, %d failed tests, %d failed asserts\n",
                  totalTests, myCUnitAssertsRun, totalFailedTests, myCUnitAssertsFailed);

    myCUnitTotalTestsFailed = (unsigned int)totalFailedTests;
}

static inline unsigned int CU_get_number_of_tests_failed(void)
{
    return myCUnitTotalTestsFailed;
}

#endif
