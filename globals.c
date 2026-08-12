#include "common.h"

/* Single point of truth for the app's in-memory data.
   Every module reads/writes these via the extern
   declarations in common.h */

Transaction transactions[MAX];
int transactionCount = 0;

Budget budgets[MAX_BUDGETS];
int budgetCount = 0;

SavingsGoal goals[MAX_GOALS];
int goalCount = 0;

char currentUser[30];

float emergencyFundBalance = 0;

pthread_mutex_t dataMutex = PTHREAD_MUTEX_INITIALIZER;
