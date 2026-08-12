#include <stdio.h>
#include <string.h>

#include "dashboard.h"
#include "common.h"
#include "budget.h"

void financialDashboard(void)
{
    float income = 0.0f;
    float expense = 0.0f;
    float savings;
    int i;
    int overBudgetCount = 0;

    for(i = 0; i < transactionCount; i++)
    {
        if(strcmp(transactions[i].type, "Income") == 0)
        {
            income += transactions[i].amount;
        }
        else
        {
            expense += transactions[i].amount;
        }
    }

    savings = income - expense;

    for(i = 0; i < budgetCount; i++)
    {
        if(getSpentForCategory(budgets[i].category) > budgets[i].limit)
        {
            overBudgetCount++;
        }
    }

    (void)printf("\n=================================================\n");
    (void)printf("             FINANCIAL DASHBOARD\n");
    (void)printf("=================================================\n");
    (void)printf("User             : %s\n", currentUser);
    (void)printf("Total Income     : %.2f\n", income);
    (void)printf("Total Expense    : %.2f\n", expense);
    (void)printf("Net Savings      : %.2f\n", savings);

    if(income > 0.0f)
    {
        (void)printf("Savings Rate     : %.2f%%\n", (savings / income) * 100.0f);
    }

    (void)printf("Budgets Set      : %d (%d over limit)\n", budgetCount, overBudgetCount);
    (void)printf("Savings Goals    : %d\n", goalCount);
    (void)printf("Total Records    : %d\n", transactionCount);
    (void)printf("=================================================\n");
}
