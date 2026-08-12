#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

/* =====================================================
   Shared helpers for the CUnit test suite.

   The application's functions (registerUser, addIncome,
   setBudget, ...) are all void(void) functions that read
   directly from stdin and print directly to stdout, and
   that operate on the global arrays declared in common.h
   (transactions[], budgets[], goals[], currentUser).

   To unit test them without changing the application code,
   each test:
     1. Resets the global state       -> th_reset_globals()
     2. Feeds the function's expected keyboard input
                                        -> th_feed_stdin()
     3. Captures whatever it prints    -> th_capture_stdout_start()
                                          / th_capture_stdout_end()
     4. Calls the function under test directly
     5. Asserts on the global state and/or the captured text
   ===================================================== */

/* Clears transactions[]/transactionCount, budgets[]/budgetCount,
   goals[]/goalCount and currentUser back to an empty, known state.
   Call this at the start of every test (or in a suite's init
   function) so tests never leak state into one another. */
void th_reset_globals(void);

/* Redirects stdin so the next fgets()/scanf() calls made by the
   code under test read from `input` exactly as if a user had
   typed it (include the trailing '\n' for each line, since
   readLine()/readValidInt()/readValidFloat() all expect it). */
void th_feed_stdin(const char *input);

/* Starts capturing everything written to stdout from this point
   on. Must be paired with a later call to th_capture_stdout_end(). */
void th_capture_stdout_start(void);

/* Stops capturing, restores the real stdout, and returns a
   newly malloc'd, NUL-terminated string containing everything
   that was printed since th_capture_stdout_start(). The caller
   owns the returned pointer and must free() it. */
char *th_capture_stdout_end(void);

/* Convenience wrapper: runs fn() with stdout captured and returns
   the captured text (caller must free() it). Useful for a setup
   step whose output isn't being asserted on, e.g.:
       free(th_call_capturing(registerUser));
   instead of a separate start/call/end for that step. */
char *th_call_capturing(void (*fn)(void));

/* Creates a fresh empty temporary directory and chdir()s into it,
   so persistence-related tests (which create <user>_*.dat /
   users.dat files as a side effect) never touch, depend on, or
   collide with real project files or each other. Pairs with
   th_leave_tmp_dir(). */
void th_enter_tmp_dir(void);

/* Removes the temporary directory created by th_enter_tmp_dir()
   (and everything the test created inside it) and restores the
   previous working directory. */
void th_leave_tmp_dir(void);

#endif
