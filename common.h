#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>

#include "sha256.h"

/* =====================================================
   Shared structs, limits, and global data used by
   every module. Each module includes this to access
   the data it needs, but owns none of it directly -
   the actual storage lives in globals.c

   Threading: dataMutex guards every write to the shared
   arrays/counters below (transactions, budgets, goals,
   emergencyFundBalance) against the background autosave
   thread (see autosave.h). Every function that mutates
   one of these - addIncome, addExpense, editTransaction,
   deleteTransaction, setBudget, setSavingsGoal,
   contributeToGoal - takes dataMutex around the mutation
   and the immediate save*() call. Read-only functions
   (viewTransactions, reports, dashboard, ...) don't need
   it: the app is otherwise single-threaded on the main
   thread, so a read there can never run concurrently with
   another main-thread write, and the autosave thread only
   ever *reads* these arrays (to write them to disk) while
   holding the same mutex - so an unlocked read here never
   races with anything.
   ===================================================== */

#define MAX 1000
#define MAX_BUDGETS 100
#define MAX_GOALS 50

/* Passwords are never stored in plaintext. Only a per-user random
   salt and the SHA-256 hash of (salt || password) are persisted to
   data/users.dat - see authentication.c. saltHex/passwordHash are
   NUL-terminated lowercase hex strings, not raw binary, so the
   on-disk struct stays simple to read/write with fread/fwrite and
   safe to log/inspect without exposing binary digest bytes. */
typedef struct
{
    char username[30];
    char saltHex[SHA256_SALT_HEX_CHARS];
    char passwordHash[SHA256_HEX_CHARS];
} User;

typedef struct
{
    int id;
    char type[20];
    char category[30];
    float amount;
    char date[11];
} Transaction;

typedef struct
{
    char category[30];
    float limit;
} Budget;

typedef struct
{
    char name[50];
    float targetAmount;
    float savedAmount;
} SavingsGoal;

extern Transaction transactions[MAX];
extern int transactionCount;

extern Budget budgets[MAX_BUDGETS];
extern int budgetCount;

extern SavingsGoal goals[MAX_GOALS];
extern int goalCount;

extern char currentUser[30];

extern float emergencyFundBalance;

extern pthread_mutex_t dataMutex;

#endif
