#ifndef ACTIVITY_LOG_H
#define ACTIVITY_LOG_H

#define ACTIVITY_LOG_CAPACITY 20

/* =====================================================
   A bounded, most-recent-first activity history.

   Implemented the way you'd do it on a target with no
   heap: a fixed-size pool of nodes, with TWO singly-linked
   lists threaded through that same pool via array indices
   instead of pointers -
     - the IN-USE list (most recent entry first)
     - the FREE list (unused slots available for reuse)
   See activity_log.c for the full explanation. No malloc
   or free anywhere in this module.
   ===================================================== */

/* Records one line of activity (e.g. "Added Expense: Food
   120.00") into the log. Once the log is full, the oldest
   entry is silently reclaimed to make room - the log never
   grows past ACTIVITY_LOG_CAPACITY entries. */
void logActivity(const char *message);

/* Prints the log, most recent entry first. */
void printActivityLog(void);

/* Clears the log back to empty. Exposed mainly so tests
   start from a known state; this only affects the in-memory
   log, it never touches disk. */
void resetActivityLog(void);

#endif
