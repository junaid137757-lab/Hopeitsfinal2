#include <stdio.h>

#include "notification.h"

void notifyBudgetStatus(const char *category, float spent, float limit)
{
    if(spent > limit)
    {
        (void)printf("\n[ALERT] You have exceeded your budget for '%s'! "
               "(Spent: %.2f / Limit: %.2f)\n",
               category, spent, limit);
    }
    else if((limit > 0.0f) && (spent >= (0.9f * limit)))
    {
        (void)printf("\n[WARNING] You are nearing your budget limit for '%s'! "
               "(Spent: %.2f / Limit: %.2f)\n",
               category, spent, limit);
    }
    else
    {
        /* Within budget - no notification needed. */
    }
}

void notifyGoalAchieved(const char *goalName)
{
    (void)printf("[CONGRATULATIONS] You have reached your goal '%s'!\n", goalName);
}
