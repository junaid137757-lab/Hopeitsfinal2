#include <pthread.h>
#include <unistd.h>

#include "autosave.h"
#include "common.h"
#include "persistence.h"
#include "logger.h"

/* How often the background thread writes everything to disk.
   Kept short (5s) so it's easy to demonstrate/observe during a
   demo session; a real deployment might use a minute or more.
   Overridable via autosaveSetIntervalSecondsForTesting() so unit
   tests don't have to wait out the real interval. */
static int autosaveIntervalSeconds = 5;

/* stopRequested is only ever read/written while holding
   stopMutex, in both this thread and the main thread - a plain
   flag without a lock would be a data race under the C11/POSIX
   threading memory model (and exactly what ThreadSanitizer /
   Helgrind exist to catch), even though in practice int writes
   are usually atomic on common platforms. */
static pthread_mutex_t stopMutex = PTHREAD_MUTEX_INITIALIZER;
static int stopRequested = 0;
static pthread_t workerThread;
static int threadRunning = 0;

static int shouldStop(void)
{
    int stop;

    (void)pthread_mutex_lock(&stopMutex);
    stop = stopRequested;
    (void)pthread_mutex_unlock(&stopMutex);

    return stop;
}

static void *autosaveWorker(void *arg)
{
    (void)arg;

    while(shouldStop() == 0)
    {
        int waited = 0;

        /* Sleep in 1-second increments rather than one long
           sleep(autosaveIntervalSeconds), so a stop request
           takes effect within ~1s instead of waiting out the
           whole interval - matters for a clean, prompt logout. */
        while((waited < autosaveIntervalSeconds) && (shouldStop() == 0))
        {
            (void)sleep(1U);
            waited++;
        }

        if(shouldStop() == 0)
        {
            (void)pthread_mutex_lock(&dataMutex);
            saveTransactions();
            saveBudgets();
            saveGoals();
            saveEmergencyFund();
            (void)pthread_mutex_unlock(&dataMutex);

            LOG_DEBUG_MSG("autosave: wrote transactions/budgets/goals/emergency fund to disk");
        }
    }

    return NULL;
}

int startAutosaveThread(void)
{
    int result;

    (void)pthread_mutex_lock(&stopMutex);
    stopRequested = 0;
    (void)pthread_mutex_unlock(&stopMutex);

    if(pthread_create(&workerThread, NULL, autosaveWorker, NULL) != 0)
    {
        LOG_ERROR_MSG("failed to start autosave thread");
        result = 0;
    }
    else
    {
        threadRunning = 1;
        LOG_INFO_MSG("autosave thread started");
        result = 1;
    }

    return result;
}

void stopAutosaveThread(void)
{
    if(threadRunning != 0)
    {
        (void)pthread_mutex_lock(&stopMutex);
        stopRequested = 1;
        (void)pthread_mutex_unlock(&stopMutex);

        (void)pthread_join(workerThread, NULL);
        threadRunning = 0;

        LOG_INFO_MSG("autosave thread stopped");
    }
}

void autosaveSetIntervalSecondsForTesting(int seconds)
{
    if(seconds > 0)
    {
        autosaveIntervalSeconds = seconds;
    }
}
