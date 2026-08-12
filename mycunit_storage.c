#include "CUnit.h"

MyCUnitSuite myCUnitSuites[MYCUNIT_MAX_SUITES];
int myCUnitSuiteCount = 0;
int myCUnitAssertsRun = 0;
int myCUnitAssertsFailed = 0;
int myCUnitCurrentTestFailed = 0;
unsigned int myCUnitTotalTestsFailed = 0;
const char *myCUnitCurrentFile = "";
int myCUnitCurrentLine = 0;
