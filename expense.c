#include <stdio.h>
#include <string.h>

#include "expense.h"
#include "common.h"
#include "utility.h"
#include "persistence.h"
#include "budget.h"
#include "activity_log.h"
#include "transaction.h"

void addExpense(void)
{
    Transaction t;
    int cancelled = 0;

    if(transactionCount >= MAX)
    {
        (void)printf("Transaction Limit Reached!\n");
        cancelled = 1;
    }

    if(cancelled == 0)
    {
        (void)strcpy(t.type, "Expense");

        (void)printf("Enter Expense Category (or 0 to cancel): ");
        readLine(t.category, (int)sizeof(t.category));

        if(strcmp(t.category, "0") == 0)
        {
            (void)printf("Add Expense Cancelled.\n");
            cancelled = 1;
        }
    }

    if(cancelled == 0)
    {
        (void)printf("Enter Amount (or -1 to cancel): ");

        if(readValidFloat(&t.amount) == 0)
        {
            cancelled = 1;
        }
        else if(t.amount == -1.0f)
        {
            (void)printf("Add Expense Cancelled.\n");
            cancelled = 1;
        }
        else
        {
            /* valid amount entered */
        }
    }

    if(cancelled == 0)
    {
        char logMsg[80];

        getCurrentDate(t.date);

        (void)pthread_mutex_lock(&dataMutex);
        t.id = nextTransactionId();
        transactions[transactionCount] = t;
        transactionCount++;
        saveTransactions();
        (void)pthread_mutex_unlock(&dataMutex);

        (void)printf("Expense Added Successfully!\n");

        (void)snprintf(logMsg, sizeof(logMsg), "Added Expense: %s %.2f", t.category, t.amount);
        logActivity(logMsg);

        checkBudgetAlert(t.category);
    }
}
