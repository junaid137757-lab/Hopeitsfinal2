#include <stdio.h>
#include <string.h>

#include "reports.h"
#include "common.h"
#include "transaction.h"

void categoryWiseReport(void)
{
    if(transactionCount == 0)
    {
        (void)printf("No Transactions Available\n");
    }
    else
    {
        static char seen[MAX][30];
        static float totals[MAX];
        int seenCount = 0;
        int i;
        int j;
        float remainingBalance = 0.0f;

        sortTransactionsByDate();

        for(i = 0; i < transactionCount; i++)
        {
            int found = -1;
            float signedAmount = (strcmp(transactions[i].type, "Income") == 0)
                                      ? transactions[i].amount
                                      : -transactions[i].amount;

            for(j = 0; j < seenCount; j++)
            {
                if(strcmp(seen[j], transactions[i].category) == 0)
                {
                    found = j;
                }
            }

            if(found == -1)
            {
                (void)strcpy(seen[seenCount], transactions[i].category);
                totals[seenCount] = signedAmount;
                seenCount++;
            }
            else
            {
                totals[found] += signedAmount;
            }
        }

        (void)printf("\n--------------------------------------------------------------------\n");
        (void)printf("CATEGORY-WISE REPORT (Positive = Net Income, Negative = Net Expense)\n");
        (void)printf("--------------------------------------------------------------------\n");
        (void)printf("%-20s%s\n", "CATEGORY", "NET AMOUNT");
        (void)printf("--------------------------------------------------------------------\n");

        for(i = 0; i < seenCount; i++)
        {
            (void)printf("%-20s%.2f\n", seen[i], totals[i]);
            remainingBalance += totals[i];
        }

        (void)printf("--------------------------------------------------------------------\n");
        (void)printf("Remaining Balance (Total Income - Total Expense): %.2f\n", remainingBalance);
    }
}

void monthlyReport(void)
{
    if(transactionCount == 0)
    {
        (void)printf("No Transactions Available\n");
    }
    else
    {
        static char months[MAX][8];
        static float income[MAX];
        static float expense[MAX];
        int monthCount = 0;
        int i;
        int j;

        sortTransactionsByDate();

        for(i = 0; i < transactionCount; i++)
        {
            char monthKey[8];
            int found = -1;

            (void)strncpy(monthKey, transactions[i].date, 7U);
            monthKey[7] = '\0';

            for(j = 0; j < monthCount; j++)
            {
                if(strcmp(months[j], monthKey) == 0)
                {
                    found = j;
                }
            }

            if(found == -1)
            {
                (void)strcpy(months[monthCount], monthKey);
                income[monthCount] = 0.0f;
                expense[monthCount] = 0.0f;
                found = monthCount;
                monthCount++;
            }

            if(strcmp(transactions[i].type, "Income") == 0)
            {
                income[found] += transactions[i].amount;
            }
            else
            {
                expense[found] += transactions[i].amount;
            }
        }

        (void)printf("\n--------------------------------------------------------------------\n");
        (void)printf("MONTHLY REPORT\n");
        (void)printf("--------------------------------------------------------------------\n");
        (void)printf("%-12s%-14s%-14s%s\n", "MONTH", "INCOME", "EXPENSE", "NET");
        (void)printf("--------------------------------------------------------------------\n");

        for(i = 0; i < monthCount; i++)
        {
            (void)printf("%-12s%-14.2f%-14.2f%.2f\n",
                   months[i], income[i], expense[i], income[i] - expense[i]);
        }
    }
}
