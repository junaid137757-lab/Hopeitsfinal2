#include <stdio.h>
#include <string.h>

#include "recommendation.h"
#include "common.h"
#include "budget.h"

void generateRecommendations(void)
{
    float income = 0.0f;
    float expense = 0.0f;
    int i;
    int hasEmergencyFund = 0;
    int printedAny = 0;

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

    for(i = 0; i < goalCount; i++)
    {
        if((strstr(goals[i].name, "Emergency") != NULL) ||
           (strstr(goals[i].name, "emergency") != NULL))
        {
            hasEmergencyFund = 1;
        }
    }

    (void)printf("\n=================================================\n");
    (void)printf("        SMART FINANCIAL RECOMMENDATIONS\n");
    (void)printf("=================================================\n");

    if(transactionCount == 0)
    {
        (void)printf("Start logging your income and expenses to receive\n");
        (void)printf("personalized financial recommendations.\n");
    }
    else
    {
        if(expense > income)
        {
            (void)printf("- Your expenses currently exceed your income. Consider\n");
            (void)printf("  reviewing your largest spending categories and cutting\n");
            (void)printf("  back to avoid debt.\n");
            printedAny = 1;
        }
        else if((income > 0.0f) && (((income - expense) / income) < 0.20f))
        {
            (void)printf("- Your savings rate is below 20%%. Try to trim discretionary\n");
            (void)printf("  spending to build a healthier savings buffer.\n");
            printedAny = 1;
        }
        else if(income > 0.0f)
        {
            (void)printf("- Great job! Your savings rate is healthy. Consider investing\n");
            (void)printf("  your surplus savings for long-term growth.\n");
            printedAny = 1;
        }
        else
        {
            /* no income recorded yet - nothing to say about savings rate */
        }

        for(i = 0; i < budgetCount; i++)
        {
            float spent = getSpentForCategory(budgets[i].category);

            if(spent > budgets[i].limit)
            {
                (void)printf("- You are over budget in '%s'. Consider lowering spending\n", budgets[i].category);
                (void)printf("  in this category next month.\n");
                printedAny = 1;
            }
        }

        for(i = 0; i < goalCount; i++)
        {
            if((goals[i].targetAmount > 0.0f) &&
               ((goals[i].savedAmount / goals[i].targetAmount) < 0.5f))
            {
                (void)printf("- Your goal '%s' is less than halfway funded. Consider\n", goals[i].name);
                (void)printf("  setting up regular contributions.\n");
                printedAny = 1;
            }
        }

        if(hasEmergencyFund == 0)
        {
            (void)printf("- You don't have an emergency fund goal yet. Consider creating\n");
            (void)printf("  one to cover 3-6 months of essential expenses.\n");
            printedAny = 1;
        }

        if(printedAny == 0)
        {
            (void)printf("- Your finances look well managed. Keep up the good habits!\n");
        }
    }

    (void)printf("=================================================\n");
}
