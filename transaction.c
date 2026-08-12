#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "transaction.h"
#include "common.h"
#include "persistence.h"
#include "budget.h"
#include "utility.h"
#include "activity_log.h"
#include "logger.h"

/* Comparator for qsort(): orders transactions chronologically
   by date. Dates are stored as "YYYY-MM-DD" strings, which
   happen to sort correctly with a plain strcmp() because the
   format is fixed-width and most-significant-field-first -
   no need to parse them into a struct tm just to compare.

   MISRA-C:2012 Rule 11.5 deviation: qsort()'s comparator
   interface requires a void* parameter type - see
   MISRA_DEVIATIONS.md. */
static int compareByDate(const void *a, const void *b)
{
    const Transaction *ta = (const Transaction *)a;
    const Transaction *tb = (const Transaction *)b;

    return strcmp(ta->date, tb->date);
}

int nextTransactionId(void)
{
    int maxId = 0;
    int i;

    for(i = 0; i < transactionCount; i++)
    {
        if(transactions[i].id > maxId)
        {
            maxId = transactions[i].id;
        }
    }

    return maxId + 1;
}

void renumberTransactionIds(void)
{
    int i;

    for(i = 0; i < transactionCount; i++)
    {
        transactions[i].id = i + 1;
    }
}

void sortTransactionsByDate(void)
{
    qsort(transactions, (size_t)transactionCount, sizeof(Transaction), compareByDate);
}

/* Comparator for qsort(): orders transactions by amount,
   smallest first. Uses explicit comparisons rather than
   subtracting the floats, since a bare subtraction can't be
   safely truncated to an int the way it can for integer keys. */
static int compareByAmount(const void *a, const void *b)
{
    const Transaction *ta = (const Transaction *)a;
    const Transaction *tb = (const Transaction *)b;
    int result;

    if(ta->amount < tb->amount)
    {
        result = -1;
    }
    else if(ta->amount > tb->amount)
    {
        result = 1;
    }
    else
    {
        result = 0;
    }

    return result;
}

void sortTransactionsByAmount(void)
{
    qsort(transactions, (size_t)transactionCount, sizeof(Transaction), compareByAmount);
}

void viewTransactions(void)
{
    if(transactionCount == 0)
    {
        (void)printf("No Transactions Available\n");
    }
    else
    {
        int i;
        float balance = 0.0f;

        (void)printf("\n--------------------------------------------------------------------------------\n");
        (void)printf("%-5s%-10s%-16s%-12s%-14s%-12s\n", "ID", "TYPE", "CATEGORY", "AMOUNT", "DATE", "BALANCE");
        (void)printf("--------------------------------------------------------------------------------\n");

        for(i = 0; i < transactionCount; i++)
        {
            if(strcmp(transactions[i].type, "Income") == 0)
            {
                balance += transactions[i].amount;
            }
            else
            {
                balance -= transactions[i].amount;
            }

            (void)printf("%-5d%-10s%-16s%-12.2f%-14s%-12.2f\n",
                   transactions[i].id,
                   transactions[i].type,
                   transactions[i].category,
                   transactions[i].amount,
                   transactions[i].date,
                   balance);
        }

        (void)printf("--------------------------------------------------------------------------------\n");
        (void)printf("Running Balance (Total Income - Total Expense): %.2f\n", balance);
    }
}

void editTransaction(void)
{
    int id = 0;
    int found = -1;
    char newCategory[30] = "";
    float newAmount = 0.0f;
    int cancelled = 0;

    viewTransactions();

    if(transactionCount == 0)
    {
        cancelled = 1;
    }

    if(cancelled == 0)
    {
        (void)printf("\nEnter ID of Transaction to Edit (or 0 to cancel): ");

        if(readValidInt(&id) == 0)
        {
            cancelled = 1;
        }
        else if(id == 0)
        {
            (void)printf("Edit Cancelled.\n");
            cancelled = 1;
        }
        else
        {
            /* valid id entered */
        }
    }

    if(cancelled == 0)
    {
        int i;

        for(i = 0; i < transactionCount; i++)
        {
            if(transactions[i].id == id)
            {
                found = i;
            }
        }

        if(found == -1)
        {
            (void)printf("Transaction Not Found!\n");
            cancelled = 1;
        }
    }

    if(cancelled == 0)
    {
        (void)printf("Enter New Category (or 0 to cancel): ");
        readLine(newCategory, (int)sizeof(newCategory));

        if(strcmp(newCategory, "0") == 0)
        {
            (void)printf("Edit Cancelled.\n");
            cancelled = 1;
        }
    }

    if(cancelled == 0)
    {
        (void)printf("Enter New Amount (or -1 to cancel): ");

        if(readValidFloat(&newAmount) == 0)
        {
            cancelled = 1;
        }
        else if(newAmount == -1.0f)
        {
            (void)printf("Edit Cancelled.\n");
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
        (void)strcpy(transactions[found].category, newCategory);
        transactions[found].amount = newAmount;
        saveTransactions();
        (void)pthread_mutex_unlock(&dataMutex);

        (void)printf("Transaction Updated Successfully!\n");

        (void)snprintf(logMsg, sizeof(logMsg), "Edited Transaction #%d: %s %.2f", id, newCategory, newAmount);
        logActivity(logMsg);

        if(strcmp(transactions[found].type, "Expense") == 0)
        {
            checkBudgetAlert(transactions[found].category);
        }
    }
}

void deleteTransaction(void)
{
    int id = 0;
    int found = -1;
    int cancelled = 0;

    viewTransactions();

    if(transactionCount == 0)
    {
        cancelled = 1;
    }

    if(cancelled == 0)
    {
        (void)printf("\nEnter ID of Transaction to Delete (or 0 to cancel): ");

        if(readValidInt(&id) == 0)
        {
            cancelled = 1;
        }
        else if(id == 0)
        {
            (void)printf("Delete Cancelled.\n");
            cancelled = 1;
        }
        else
        {
            /* valid id entered */
        }
    }

    if(cancelled == 0)
    {
        int i;

        for(i = 0; i < transactionCount; i++)
        {
            if(transactions[i].id == id)
            {
                found = i;
            }
        }

        if(found == -1)
        {
            char logMsg[80];

            (void)snprintf(logMsg, sizeof(logMsg), "delete requested for unknown transaction id %d", id);
            LOG_WARN_MSG(logMsg);

            (void)printf("Transaction Not Found!\n");
            cancelled = 1;
        }
    }

    if((cancelled == 0) && (found != -1))
    {
        char logMsg[80];

        (void)snprintf(logMsg, sizeof(logMsg), "Deleted Transaction: %s %s %.2f",
                 transactions[found].type, transactions[found].category, transactions[found].amount);

        (void)pthread_mutex_lock(&dataMutex);

        {
            int i;

            for(i = found; i < (transactionCount - 1); i++)
            {
                transactions[i] = transactions[i + 1];
            }
        }

        transactionCount--;
        renumberTransactionIds();

        saveTransactions();

        (void)pthread_mutex_unlock(&dataMutex);

        (void)printf("Transaction Deleted Successfully!\n");

        logActivity(logMsg);
    }
}

static int printMatchingTransactions(const char *category)
{
    int i;
    int found = 0;

    for(i = 0; i < transactionCount; i++)
    {
        if(strcmp(transactions[i].category, category) == 0)
        {
            if(found == 0)
            {
                (void)printf("\n--------------------------------------------------------------------\n");
                (void)printf("%-5s%-10s%-16s%-12s%-14s\n", "ID", "TYPE", "CATEGORY", "AMOUNT", "DATE");
                (void)printf("--------------------------------------------------------------------\n");
            }

            (void)printf("%-5d%-10s%-16s%-12.2f%-14s\n",
                   transactions[i].id,
                   transactions[i].type,
                   transactions[i].category,
                   transactions[i].amount,
                   transactions[i].date);

            found = 1;
        }
    }

    return found;
}

void searchTransactionsByCategory(void)
{
    char category[30] = "";
    int cancelled = 0;

    (void)printf("Enter Category to Search (or 0 to cancel): ");
    readLine(category, (int)sizeof(category));

    if(strcmp(category, "0") == 0)
    {
        (void)printf("Search Cancelled.\n");
        cancelled = 1;
    }

    if(cancelled == 0)
    {
        static char suggestions[10][30];
        int suggestionCount = 0;
        int i;
        int j;
        int suggestionChoice = 0;
        int exactMatchFound;

        exactMatchFound = printMatchingTransactions(category);

        if(exactMatchFound == 0)
        {
            (void)printf("No Transactions Found In This Category\n");

            /* No exact match - offer "Did you mean" suggestions based on a
               case-insensitive partial match, the way a search engine would,
               instead of just giving up (e.g. searching "income" when the
               actual category is "stocks income"). */
            for(i = 0; (i < transactionCount) && (suggestionCount < 10); i++)
            {
                int alreadyListed = 0;

                for(j = 0; j < suggestionCount; j++)
                {
                    if(strcmp(suggestions[j], transactions[i].category) == 0)
                    {
                        alreadyListed = 1;
                    }
                }

                if((alreadyListed == 0) &&
                   ((caseInsensitiveContains(transactions[i].category, category) != 0) ||
                    (caseInsensitiveContains(category, transactions[i].category) != 0)))
                {
                    (void)strcpy(suggestions[suggestionCount], transactions[i].category);
                    suggestionCount++;
                }
            }

            if(suggestionCount > 0)
            {
                (void)printf("\nDid you mean:\n");

                for(i = 0; i < suggestionCount; i++)
                {
                    (void)printf("  %d. %s\n", i + 1, suggestions[i]);
                }

                (void)printf("Enter number to search that category (or 0 to skip): ");

                if(readValidInt(&suggestionChoice) != 0)
                {
                    if((suggestionChoice >= 1) && (suggestionChoice <= suggestionCount))
                    {
                        (void)printMatchingTransactions(suggestions[suggestionChoice - 1]);
                    }
                }
            }
        }
    }
}
