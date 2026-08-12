#ifndef LOGGER_H
#define LOGGER_H

/* =====================================================
   File-based logging module: levels, timestamps, size-
   based rotation, and a debug/production switch.

   This is separate from activity_log.c on purpose:
   activity_log.c is an in-memory, most-recent-first feed
   of user-facing actions ("Added Expense: Food 120.00")
   shown inside the app. logger.c is a persistent, leveled
   trace log written to disk, meant for developers/support
   diagnosing a run after the fact.

   MISRA-C:2012 note: Rule 17.1 forbids <stdarg.h> / variadic
   functions, so unlike a typical printf-style logger this
   module takes a single, already-formatted message string.
   Callers build the message with snprintf() into a local
   buffer first - exactly the same pattern already used for
   activity_log's logActivity() calls throughout this codebase
   - then pass the buffer in. See any call site for the pattern.
   ===================================================== */

typedef enum
{
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3
} LogLevel;

/* Opens (creates if needed) the log file at path and makes
   the module ready to log. Sets the initial minimum level:
   LOG_DEBUG in a debug build, LOG_INFO in a production
   build (see DEBUG_BUILD below). Safe to call once at
   program startup. Returns 1 on success, 0 if the file
   could not be opened. */
int logInit(const char *path);

/* Changes the minimum level that gets written. Anything
   below this level is silently skipped. Lets a caller
   override the debug/production default at runtime, e.g.
   a "--verbose" flag. */
void logSetLevel(LogLevel level);

/* Writes one line: "[TIMESTAMP] [LEVEL] [file:line func()] message".
   `message` must already be a complete, formatted string (build
   it with snprintf() at the call site - see LOG_*_MSG below). */
void logMessage(LogLevel level, const char *file, int line,
                 const char *func, const char *message);

/* Flushes and closes the log file. Call once at shutdown. */
void logClose(void);

/* --- convenience macros -------------------------------
   These capture the call site automatically, which is what
   gives you "error tracing capability": every ERROR/WARN
   line in app.log shows exactly which file/function/line
   raised it, not just the message. Pass a plain string, e.g.:

       char logMsg[80];
       snprintf(logMsg, sizeof(logMsg), "user '%s' logged in", username);
       LOG_INFO_MSG(logMsg);
*/
#define LOG_DEBUG_MSG(msg) logMessage(LOG_DEBUG, __FILE__, __LINE__, __func__, (msg))
#define LOG_INFO_MSG(msg)  logMessage(LOG_INFO,  __FILE__, __LINE__, __func__, (msg))
#define LOG_WARN_MSG(msg)  logMessage(LOG_WARN,  __FILE__, __LINE__, __func__, (msg))
#define LOG_ERROR_MSG(msg) logMessage(LOG_ERROR, __FILE__, __LINE__, __func__, (msg))

#endif
