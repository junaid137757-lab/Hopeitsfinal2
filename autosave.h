#ifndef AUTOSAVE_H
#define AUTOSAVE_H

/* =====================================================
   Background autosave thread.

   While a user is logged in, this periodically saves
   transactions/budgets/goals/emergency fund to disk on
   its own thread, so a crash or power loss mid-session
   loses at most one interval's worth of changes instead
   of everything since the last explicit save.

   Every access this thread makes to the shared data
   (transactions[], budgets[], goals[], emergencyFundBalance)
   is made while holding dataMutex (declared in common.h) -
   the same lock the main thread's mutating functions
   (addIncome, addExpense, editTransaction, deleteTransaction,
   setBudget, setSavingsGoal, contributeToGoal) take around
   their own writes to that data, so the two threads can never
   touch it at the same time.
   ===================================================== */

/* Starts the autosave thread. Call once, after a successful
   login and after the initial load*() calls. Returns 1 on
   success, 0 if the thread could not be created (logs the
   reason either way). */
int startAutosaveThread(void);

/* Signals the autosave thread to stop and blocks until it has
   actually exited (pthread_join). Call once, before doing a
   final manual save on logout - by the time this returns, the
   autosave thread is guaranteed to have already finished any
   save it was doing, so the caller's own save afterward needs
   no locking. Safe to call even if the thread was never
   started, or was already stopped. */
void stopAutosaveThread(void);

/* Test-only hook: overrides the autosave interval (production
   default: 5 seconds) so tests can observe a save cycle without
   waiting several seconds per test. Not used by main.c. */
void autosaveSetIntervalSecondsForTesting(int seconds);

#endif
