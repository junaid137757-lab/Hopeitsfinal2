#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "logger.h"

/* Rotate once the log file passes this size. Kept small here
   (50 KB) so rotation is easy to demo/test; bump it up for a
   real deployment. */
#define LOG_MAX_BYTES  (50U * 1024U)
#define LOG_PATH_MAX   256

/* Guards every static below (logFile, logPath, currentLevel) so
   the logger is safe to call from multiple threads at once - the
   main thread and the background autosave thread (see autosave.c)
   both log through here concurrently. logMessage()/logInit()/
   logClose()/logSetLevel() are the only public entry points and
   each takes this lock for its whole body. logMessage() itself is
   split into a public locking wrapper and an internal *Impl()
   that assumes the lock is already held, so logInit()/logClose()
   can log their own "started"/"stopped" line without relocking
   (a plain, non-recursive mutex would deadlock on that). */
static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;

static FILE *logFile = NULL;
static char logPath[LOG_PATH_MAX];

#ifdef DEBUG_BUILD
static LogLevel currentLevel = LOG_DEBUG;
#else
static LogLevel currentLevel = LOG_INFO;
#endif

static const char *levelName(LogLevel level)
{
    const char *name;

    switch(level)
    {
        case LOG_DEBUG: name = "DEBUG";   break;
        case LOG_INFO:  name = "INFO";    break;
        case LOG_WARN:  name = "WARN";    break;
        case LOG_ERROR: name = "ERROR";   break;
        default:        name = "UNKNOWN"; break;
    }

    return name;
}

/* "YYYY-MM-DD HH:MM:SS" - same style as utility.c's
   getCurrentDate(), just with a time component added since
   a log needs to order and correlate events within a day.
   Uses localtime_r rather than localtime(): localtime() writes
   through a single static struct tm shared by the whole process,
   which is unsafe if two threads call it at once - localtime_r
   fills a caller-supplied struct instead, so it's reentrant. */
static void currentTimestamp(char *buffer, size_t size)
{
    time_t t = time(NULL);
    struct tm tm_info;

    (void)localtime_r(&t, &tm_info);
    (void)strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &tm_info);
}

/* If the file has grown past LOG_MAX_BYTES, close it, rename
   it to "<path>.old" (overwriting any previous .old), and
   reopen a fresh empty file at the original path. Keeping one
   rotated backup is the simplest scheme that still stops the
   log growing unbounded - swap this for numbered/dated backups
   if you want more history. Caller must hold logMutex. */
static void rotateIfNeeded(void)
{
    if(logFile != NULL)
    {
        long size;

        (void)fflush(logFile);

        /* An explicit seek-to-end is required here, not just ftell().
           A stream opened in append ("a") mode can report a stale/zero
           position from ftell() until this process has written to it
           at least once - the OS always appends writes at the true EOF
           regardless, but the FILE*'s tracked position can lag behind
           that on reopen. Seeking to SEEK_END first forces ftell() to
           report the real current file size, so rotation is detected
           correctly even for a log file that was already large before
           this process/session started. */
        (void)fseek(logFile, 0, SEEK_END);
        size = ftell(logFile);

        if(size >= (long)LOG_MAX_BYTES)
        {
            char oldPath[LOG_PATH_MAX + 4U];

            (void)fclose(logFile);

            (void)snprintf(oldPath, sizeof(oldPath), "%s.old", logPath);
            (void)remove(oldPath);
            (void)rename(logPath, oldPath);

            logFile = fopen(logPath, "a");
        }
    }
}

/* Does the actual write. Caller must already hold logMutex. */
static void logMessageImpl(LogLevel level, const char *file, int line,
                            const char *func, const char *message)
{
    if((logFile != NULL) && (level >= currentLevel))
    {
        char timestamp[20];

        rotateIfNeeded();

        currentTimestamp(timestamp, sizeof(timestamp));

        (void)fprintf(logFile, "[%s] [%-5s] [%s:%d %s()] %s\n",
                       timestamp, levelName(level), file, line, func, message);

        (void)fflush(logFile); /* flush every line so a crash doesn't lose the trace */
    }
}

int logInit(const char *path)
{
    int ok = 0;

    (void)pthread_mutex_lock(&logMutex);

    (void)strncpy(logPath, path, sizeof(logPath) - 1U);
    logPath[sizeof(logPath) - 1U] = '\0';

    logFile = fopen(logPath, "a");

    if(logFile != NULL)
    {
        ok = 1;
        logMessageImpl(LOG_INFO, __FILE__, __LINE__, "logInit", "logging started");
    }

    (void)pthread_mutex_unlock(&logMutex);

    return ok;
}

void logSetLevel(LogLevel level)
{
    (void)pthread_mutex_lock(&logMutex);
    currentLevel = level;
    (void)pthread_mutex_unlock(&logMutex);
}

void logMessage(LogLevel level, const char *file, int line,
                 const char *func, const char *message)
{
    (void)pthread_mutex_lock(&logMutex);
    logMessageImpl(level, file, line, func, message);
    (void)pthread_mutex_unlock(&logMutex);
}

void logClose(void)
{
    (void)pthread_mutex_lock(&logMutex);

    if(logFile != NULL)
    {
        logMessageImpl(LOG_INFO, __FILE__, __LINE__, "logClose", "logging stopped");
        (void)fclose(logFile);
        logFile = NULL;
    }

    (void)pthread_mutex_unlock(&logMutex);
}
