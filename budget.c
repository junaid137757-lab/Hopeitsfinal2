#include <stdio.h>
#include <string.h>

#include "budget.h"
#include "common.h"
#include "persistence.h"
#include "notification.h"
#include "utility.h"
#include "category_index.h"
#include "activity_log.h"

/* NOTE (MISRA-C:2012 deviations, documented rather than "fixed"):
   Rule 21.6 (avoid <stdio.h>) and Rule 21.9 (avoid the standard
   library qsort) are inherent to this application - it is an
   interactive console program that must print to the terminal and
   keep budgets[] sorted. Removing stdio/qsort entirely would mean
   rewriting the app as embedded firmware, which is out of scope.
   These two rules are treated as accepted deviations throughout
   this file and the rest of the project - see MISRA_DEVIATIONS.md. */

float getSpentForCategory(const char *category)
{
    float spent = 0.0f;
    int i;

    for(i = 0; i < transactionCount; i++)
    {
        if((strcmp(transactions[i].type, "Expense") == 0) &&
           (strcmp(transactions[i].category, category) == 0))
        {
            spent += transactions[i].amount;
        }
    }

    return spent;
}

/* Returns the index of the first budget whose category is
   >= the given category in the (kept-sorted) budgets[]
   array - the classic "lower bound" binary search. If every
   category is smaller, returns budgetCount, which is also
   the correct insertion point at the end of the array. */
static int budgetLowerBound(const char *category)
{
    int lo = 0;
    int hi = budgetCount; /* one past the last valid index */

    while(lo < hi)
    {
        int mid = lo + ((hi - lo) / 2);

        if(strcmp(budgets[mid].category, category) < 0)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }

    return lo;
}

int findBudgetIndex(const char *category)
{
    int pos = budgetLowerBound(category);
    int result = -1;

    if((pos < budgetCount) && (strcmp(budgets[pos].category, category) == 0))
    {
        result = pos;
    }

    return result;
}

void sortBudgetsByCategory(void)
{
    /* Simple insertion sort, in place - avoids the standard library
       qsort() (Rule 21.9) and its void* comparator callback (Rule
       11.5) entirely. budgetCount is small (bounded by MAX_BUDGETS),
       so O(n^2) is perfectly fine here; it also keeps equal-category
       entries in their original relative order (stable), which
       qsort does not guarantee. */
    int i;

    for(i = 1; i < budgetCount; i++)
    {
        Budget key = budgets[i];
        int j = i - 1;

        while((j >= 0) && (strcmp(budgets[j].category, key.category) > 0))
        {
            budgets[j + 1] = budgets[j];
            j--;
        }

        budgets[j + 1] = key;
    }
}

/* Collects the distinct transaction categories seen so far into
   existingCategories[]/existingCount. Returns 1 on success, 0 if
   there are no categories to offer. Pulled out of setBudget() so
   that function can have a single, clean exit point. */
static int collectExistingCategories(char existingCategories[][30], int *existingCount)
{
    int i;
    int j;
    int count = 0;

    for(i = 0; (i < transactionCount) && (count < 50); i++)
    {
        int alreadyListed = 0;

        for(j = 0; j < count; j++)
        {
            if(strcmp(existingCategories[j], transactions[i].category) == 0)
            {
                alreadyListed = 1;
                break;
            }
        }

        if(alreadyListed == 0)
        {
            (void)strcpy(existingCategories[count], transactions[i].category);
            count++;
        }
    }

    *existingCount = count;

    return (count > 0) ? 1 : 0;
}

/* Inserts/updates one budget entry, keeping budgets[] sorted by
   category (required by budgetLowerBound's binary search). */
static void applyBudget(const char *category, float limit)
{
    int found = findBudgetIndex(category);

    (void)pthread_mutex_lock(&dataMutex);

    if(found != -1)
    {
        budgets[found].limit = limit;
        (void)printf("Budget Updated Successfully!\n");
    }
    else
    {
        int pos = budgetLowerBound(category);
        int k;

        /* Shift everything from the insertion point right by
           one slot to make room, keeping budgets[] sorted by
           category at all times - this is what makes the
           binary search in findBudgetIndex() valid. */
        for(k = budgetCount; k > pos; k--)
        {
            budgets[k] = budgets[k - 1];
        }

        (void)strcpy(budgets[pos].category, category);
        budgets[pos].limit = limit;
        budgetCount++;

        (void)printf("Budget Set Successfully!\n");
    }

    categoryIndexRebuild();
    saveBudgets();

    (void)pthread_mutex_unlock(&dataMutex);

    {
        char logMsg[80];

        (void)snprintf(logMsg, sizeof(logMsg), "Set Budget: %s %.2f", category, (double)limit);
        logActivity(logMsg);
    }
}

void setBudget(void)
{
    char category[30];
    char existingCategories[50][30];
    int existingCount = 0;
    float limit = 0.0f;
    int choice = 0;
    int cancelled = 0;

    category[0] = '\0';

    if(budgetCount >= MAX_BUDGETS)
    {
        (void)printf("Budget Limit Reached!\n");
        cancelled = 1;
    }

    /* Only let the user set a budget for a category that's actually
       been used in a transaction, rather than free-typing a name
       that might not exist or have a typo - show the real
       categories first, then require picking one of them. */
    if((cancelled == 0) && (collectExistingCategories(existingCategories, &existingCount) == 0))
    {
        (void)printf("No transaction categories available yet. Add an income or\n");
        (void)printf("expense first, then set a budget for that category.\n");
        cancelled = 1;
    }

    if(cancelled == 0)
    {
        int i;

        (void)printf("\nExisting Categories:\n");

        for(i = 0; i < existingCount; i++)
        {
            (void)printf("  %d. %s\n", i + 1, existingCategories[i]);
        }

        (void)printf("Select a Category by Number (or 0 to cancel): ");

        if(readValidInt(&choice) == 0)
        {
            cancelled = 1;
        }
        else if(choice == 0)
        {
            (void)printf("Set Budget Cancelled.\n");
            cancelled = 1;
        }
        else if((choice < 1) || (choice > existingCount))
        {
            (void)printf("Invalid Choice\n");
            cancelled = 1;
        }
        else
        {
            (void)strcpy(category, existingCategories[choice - 1]);
        }
    }

    if(cancelled == 0)
    {
        (void)printf("Enter Monthly Budget Limit (or -1 to cancel): ");

        if(readValidFloat(&limit) == 0)
        {
            cancelled = 1;
        }
        else if(limit == -1.0f)
        {
            (void)printf("Set Budget Cancelled.\n");
            cancelled = 1;
        }
        else
        {
            /* fall through: proceed to apply below */
        }
    }

    if(cancelled == 0)
    {
        applyBudget(category, limit);
    }
}

static void printBudgetStatus(float spent, float limit)
{
    if(spent > limit)
    {
        (void)printf("[OVER BUDGET]");
    }
    else if((limit > 0.0f) && (spent >= (0.9f * limit)))
    {
        (void)printf("[NEAR LIMIT]");
    }
    else
    {
        (void)printf("[OK]");
    }
}

void viewBudgets(void)
{
    if(budgetCount == 0)
    {
        (void)printf("No Budgets Set\n");
    }
    else
    {
        int i;

        (void)printf("\n--------------------------------------------------------------------------------\n");
        (void)printf("%-16s%-12s%-12s%-14s%s\n", "CATEGORY", "LIMIT", "SPENT", "REMAINING", "STATUS");
        (void)printf("--------------------------------------------------------------------------------\n");

        for(i = 0; i < budgetCount; i++)
        {
            float spent = getSpentForCategory(budgets[i].category);
            float remaining = budgets[i].limit - spent;

            (void)printf("%-16s%-12.2f%-12.2f%-14.2f",
                   budgets[i].category,
                   (double)budgets[i].limit,
                   (double)spent,
                   (double)remaining);

            printBudgetStatus(spent, budgets[i].limit);

            (void)printf("\n");
        }
    }
}

void checkBudgetAlert(const char *category)
{
    int idx = categoryIndexLookup(category);

    if(idx != -1)
    {
        float spent = getSpentForCategory(category);

        notifyBudgetStatus(category, spent, budgets[idx].limit);
    }
}
