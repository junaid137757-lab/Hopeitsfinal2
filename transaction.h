#ifndef TRANSACTION_H
#define TRANSACTION_H

void viewTransactions(void);
void editTransaction(void);
void deleteTransaction(void);
void searchTransactionsByCategory(void);

/* Returns an id one greater than the highest id currently
   present in transactions[], or 1 if there are none. Used by
   addIncome()/addExpense() instead of transactionCount + 1:
   deleteTransaction() shifts the array and shrinks
   transactionCount without renumbering the survivors, so
   transactionCount + 1 can collide with an id that already
   exists once anything has ever been deleted. Scanning for
   the true max is O(n), which is fine at this app's scale. */
int nextTransactionId(void);

/* Reassigns every transactions[i].id to i + 1, so ids are
   always a dense 1..transactionCount sequence with no gaps
   and no duplicates, regardless of add/delete/load history.
   Called after deleteTransaction() shifts the array (so the
   survivors' ids stay compact instead of keeping stale
   values) and after loadTransactions() (so a file saved by
   an older build, before this renumbering existed, gets
   self-repaired the next time it's loaded rather than
   staying broken forever). Safe to call any time; a no-op
   on an already-compact array. */
void renumberTransactionIds(void);

/* Sorts transactions[] in place, oldest first, using the
   standard library's qsort() - O(n log n). Called by the
   Reports module before generating output so reports read
   chronologically rather than in raw insertion order. */
void sortTransactionsByDate(void);

/* Sorts transactions[] in place, smallest amount first.
   Not currently wired into any menu option - available for
   future use (e.g. "largest expenses" views) and exercised
   directly by its own test. */
void sortTransactionsByAmount(void);

#endif
