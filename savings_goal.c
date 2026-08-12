#include <stdio.h>
#include <string.h>

#include "savings_goal.h"
#include "common.h"
#include "persistence.h"
#include "notification.h"
#include "utility.h"
#include "activity_log.h"

void setSavingsGoal(void)
{
    char name[50] = "";
    float targetAmount;
    int cancelled = 0;

    if(goalCount >= MAX_GOALS)
    {
        (void)printf("Goal Limit Reached!\n");
        cancelled = 1;
    }

    if(cancelled == 0)
    {
        (void)printf("Enter Goal Name (or 0 to cancel): ");
        readLine(name, (int)sizeof(name));

        if(strcmp(name, "0") == 0)
        {
            (void)printf("Set Savings Goal Cancelled.\n");
            cancelled = 1;
        }
    }

    if(cancelled == 0)
    {
        (void)printf("Enter Target Amount (or -1 to cancel): ");

        if(readValidFloat(&targetAmount) == 0)
        {
            cancelled = 1;
        }
        else if(targetAmount == -1.0f)
        {
            (void)printf("Set Savings Goal Cancelled.\n");
            cancelled = 1;
        }
        else
        {
            /* valid target amount entered */
        }
    }

    if(cancelled == 0)
    {
        char logMsg[110];

        (void)pthread_mutex_lock(&dataMutex);
        (void)strcpy(goals[goalCount].name, name);
        goals[goalCount].targetAmount = targetAmount;
        goals[goalCount].savedAmount = 0.0f;

        goalCount++;

        saveGoals();
        (void)pthread_mutex_unlock(&dataMutex);

        (void)printf("Savings Goal Set Successfully!\n");

        (void)snprintf(logMsg, sizeof(logMsg), "Set Savings Goal: %s (Target %.2f)", name, targetAmount);
        logActivity(logMsg);
    }
}

void contributeToGoal(void)
{
    char name[50] = "";
    float amount;
    int found = -1;
    int cancelled = 0;

    viewSavingsGoals();

    if(goalCount == 0)
    {
        cancelled = 1;
    }

    if(cancelled == 0)
    {
        (void)printf("\nEnter Goal Name to Contribute To (or 0 to cancel): ");
        readLine(name, (int)sizeof(name));

        if(strcmp(name, "0") == 0)
        {
            (void)printf("Contribution Cancelled.\n");
            cancelled = 1;
        }
    }

    if(cancelled == 0)
    {
        int i;

        for(i = 0; i < goalCount; i++)
        {
            if(strcmp(goals[i].name, name) == 0)
            {
                found = i;
            }
        }

        if(found == -1)
        {
            (void)printf("Goal Not Found!\n");
            cancelled = 1;
        }
    }

    if(cancelled == 0)
    {
        (void)printf("Enter Amount to Add (or -1 to cancel): ");

        if(readValidFloat(&amount) == 0)
        {
            cancelled = 1;
        }
        else if(amount == -1.0f)
        {
            (void)printf("Contribution Cancelled.\n");
            cancelled = 1;
        }
        else
        {
            /* valid amount entered */
        }
    }

    if((cancelled == 0) && (found != -1))
    {
        char logMsg[80];

        (void)pthread_mutex_lock(&dataMutex);
        goals[found].savedAmount += amount;
        saveGoals();
        (void)pthread_mutex_unlock(&dataMutex);

        (void)printf("Contribution Added Successfully!\n");

        (void)snprintf(logMsg, sizeof(logMsg), "Contributed %.2f to Goal: %s", amount, goals[found].name);
        logActivity(logMsg);

        if(goals[found].savedAmount >= goals[found].targetAmount)
        {
            notifyGoalAchieved(goals[found].name);
        }
    }
}

void viewSavingsGoals(void)
{
    if(goalCount == 0)
    {
        (void)printf("No Savings Goals Set\n");
    }
    else
    {
        int i;

        (void)printf("\n--------------------------------------------------------------------------------\n");
        (void)printf("%-20s%-14s%-14s%s\n", "GOAL", "TARGET", "SAVED", "PROGRESS");
        (void)printf("--------------------------------------------------------------------------------\n");

        for(i = 0; i < goalCount; i++)
        {
            float progress = 0.0f;

            if(goals[i].targetAmount > 0.0f)
            {
                progress = (goals[i].savedAmount / goals[i].targetAmount) * 100.0f;
            }

            (void)printf("%-20s%-14.2f%-14.2f%.2f%%\n",
                   goals[i].name,
                   goals[i].targetAmount,
                   goals[i].savedAmount,
                   progress);
        }
    }
}
