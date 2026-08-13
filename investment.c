#include <stdio.h>
#include <string.h>

#include "investment.h"
#include "common.h"
#include "persistence.h"
#include "utility.h"

#define NUM_INVESTMENT_OPTIONS 4

/* Illustrative reference data for a POC - not real market rates.
   Percentages are a simple rule-of-thumb split, not personalized
   financial advice (see printDisclaimer).

   Stored as parallel flat arrays (struct-of-arrays) rather than an
   array of structs. cppcheck 2.4's MISRA addon has a confirmed
   Rule 9.2 false-positive on array-of-struct aggregate initializers
   containing string members - it flags the declaration regardless
   of how correctly the member braces are nested (verified: both
   braced and unbraced member forms were flagged identically at the
   same line). Flat const arrays of a single type don't hit that
   parser path, so this form checks clean under cppcheck 2.4 while
   keeping the data static, const, and read-only. */
static const char investmentNames[NUM_INVESTMENT_OPTIONS][30] =
{
    "Mutual Funds",
    "Gold",
    "Health Insurance",
    "Emergency Fund"
};

static const char investmentExamples[NUM_INVESTMENT_OPTIONS][80] =
{
    "e.g. Groww, Angel One, 5paisa",
    "e.g. Paytm Gold",
    "e.g. PolicyBazaar, Star Health Plan",
    "Personal Savings"
};

static const char investmentDescriptions[NUM_INVESTMENT_OPTIONS][150] =
{
    "A pooled investment professionally managed across stocks and bonds. Value moves with the market.",
    "Digital or physical gold, often used as a hedge against inflation. Value tracks the market gold price.",
    "A policy that covers medical expenses in exchange for a periodic premium. Protects savings from medical emergencies.",
    "Liquid personal savings set aside for unexpected expenses. Not invested in the market - available anytime."
};

static const char investmentDurations[NUM_INVESTMENT_OPTIONS][40] =
{
    "3-5 years (long-term)",
    "1-3 years",
    "Ongoing (annual renewal)",
    "N/A - always liquid"
};

static const char investmentExpectedReturns[NUM_INVESTMENT_OPTIONS][40] =
{
    "~10-12% p.a. (not guaranteed)",
    "~7-9% p.a. (not guaranteed)",
    "N/A - protection, not growth",
    "~3-4% p.a. (savings-equivalent)"
};

static const int investmentHasDisclaimer[NUM_INVESTMENT_OPTIONS] =
{
    1,
    1,
    0,
    0
};

static const float investmentAllocationPercent[NUM_INVESTMENT_OPTIONS] =
{
    50.0f,
    20.0f,
    10.0f,
    20.0f
};

static void printDisclaimer(void)
{
    (void)printf("\n----------------------------------------\n");
    (void)printf(" Disclaimer\n");
    (void)printf("----------------------------------------\n");
    (void)printf("Market-linked investments carry risk and returns are\n");
    (void)printf("not guaranteed. Figures shown are illustrative only\n");
    (void)printf("(POC placeholders), not financial advice. Please\n");
    (void)printf("consult a certified financial advisor before investing.\n");
    (void)printf("----------------------------------------\n");
}

static void emergencyFundMenu(void)
{
    int option;
    float amount;
    int done = 0;

    while(done == 0)
    {
        (void)printf("\n----------------------------------------\n");
        (void)printf(" Emergency Fund - Balance: %.2f\n", (double)emergencyFundBalance);
        (void)printf("----------------------------------------\n");
        (void)printf("1. Deposit\n");
        (void)printf("2. Withdraw\n");
        (void)printf("3. Back\n");
        (void)printf("Enter Choice: ");

        if(readValidInt(&option) == 0)
        {
            /* invalid entry - re-prompt */
        }
        else if(option == 1)
        {
            (void)printf("Enter Amount to Deposit (or -1 to cancel): ");

            if(readValidFloat(&amount) == 0)
            {
                /* invalid entry - re-prompt */
            }
            else if(amount == -1.0f)
            {
                (void)printf("Deposit Cancelled.\n");
            }
            else if(amount <= 0.0f)
            {
                (void)printf("Amount must be greater than zero.\n");
            }
            else
            {
                emergencyFundBalance += amount;
                saveEmergencyFund();
                (void)printf("Deposited Successfully! New Balance: %.2f\n", (double)emergencyFundBalance);
            }
        }
        else if(option == 2)
        {
            (void)printf("Enter Amount to Withdraw (or -1 to cancel): ");

            if(readValidFloat(&amount) == 0)
            {
                /* invalid entry - re-prompt */
            }
            else if(amount == -1.0f)
            {
                (void)printf("Withdrawal Cancelled.\n");
            }
            else if(amount <= 0.0f)
            {
                (void)printf("Amount must be greater than zero.\n");
            }
            else if(amount > emergencyFundBalance)
            {
                (void)printf("Insufficient Balance! Current Balance: %.2f\n", (double)emergencyFundBalance);
            }
            else
            {
                emergencyFundBalance -= amount;
                saveEmergencyFund();
                (void)printf("Withdrawn Successfully! New Balance: %.2f\n", (double)emergencyFundBalance);
            }
        }
        else if(option == 3)
        {
            done = 1;
        }
        else
        {
            (void)printf("Invalid Choice\n");
        }
    }
}

static void printInvestmentDetail(int index)
{
    (void)printf("\n========================================\n");
    (void)printf(" %s\n", investmentNames[index]);
    (void)printf("========================================\n");
    (void)printf("Examples         : %s\n", investmentExamples[index]);
    (void)printf("How it works     : %s\n", investmentDescriptions[index]);
    (void)printf("Typical Duration : %s\n", investmentDurations[index]);
    (void)printf("Expected Returns : %s\n", investmentExpectedReturns[index]);

    if(investmentHasDisclaimer[index] != 0)
    {
        printDisclaimer();
    }

    /* Only Emergency Fund (index 3) gets deposit/withdraw access,
       since it's the one liquid category users actively manage. */
    if(index == 3)
    {
        emergencyFundMenu();
    }
}

static void viewInvestmentCategories(void)
{
    int choice;
    int done = 0;

    while(done == 0)
    {
        (void)printf("\n----------------------------------------\n");
        (void)printf(" Investment Categories\n");
        (void)printf("----------------------------------------\n");
        (void)printf("1. Mutual Funds\n");
        (void)printf("2. Gold\n");
        (void)printf("3. Health Insurance\n");
        (void)printf("4. Emergency Funds\n");
        (void)printf("5. Back\n");
        (void)printf("Enter Choice: ");

        if(readValidInt(&choice) == 0)
        {
            /* invalid entry - re-prompt */
        }
        else if((choice >= 1) && (choice <= 4))
        {
            printInvestmentDetail(choice - 1);
        }
        else if(choice == 5)
        {
            done = 1;
        }
        else
        {
            (void)printf("Invalid Choice\n");
        }
    }
}

static void getInvestmentRecommendation(void)
{
    float amount;
    int cancelled = 0;

    (void)printf("Enter Amount to Invest (or -1 to cancel): ");

    if(readValidFloat(&amount) == 0)
    {
        cancelled = 1;
    }
    else if(amount == -1.0f)
    {
        (void)printf("Cancelled.\n");
        cancelled = 1;
    }
    else if(amount <= 0.0f)
    {
        (void)printf("Amount must be greater than zero.\n");
        cancelled = 1;
    }
    else
    {
        /* valid amount entered */
    }

    if(cancelled == 0)
    {
        int i;
        int hasMarketLinked = 0;
        char shareStr[16];

        (void)printf("\n========================================\n");
        (void)printf(" Suggested Allocation for %.2f\n", (double)amount);
        (void)printf("========================================\n");
        (void)printf("%-18s%-10s%-12s%-26s%s\n",
               "CATEGORY", "SHARE", "AMOUNT", "DURATION", "EXPECTED RETURNS");
        (void)printf("--------------------------------------------------------------------------------\n");

        for(i = 0; i < NUM_INVESTMENT_OPTIONS; i++)
        {
            float share = (amount * investmentAllocationPercent[i]) / 100.0f;

            (void)snprintf(shareStr, sizeof(shareStr), "%.0f%%", (double)investmentAllocationPercent[i]);

            (void)printf("%-18s%-10s%-12.2f%-26s%s\n",
                   investmentNames[i],
                   shareStr,
                   (double)share,
                   investmentDurations[i],
                   investmentExpectedReturns[i]);

            if(investmentHasDisclaimer[i] != 0)
            {
                hasMarketLinked = 1;
            }
        }

        (void)printf("--------------------------------------------------------------------------------\n");
        (void)printf("This is a simplified rule-of-thumb split for illustration (POC)\n");
        (void)printf("purposes only, not personalized financial advice.\n");

        if(hasMarketLinked != 0)
        {
            printDisclaimer();
        }
    }
}

void investmentMenu(void)
{
    int choice;
    int done = 0;

    while(done == 0)
    {
        (void)printf("\n========================================\n");
        (void)printf(" INVESTMENT\n");
        (void)printf("========================================\n");
        (void)printf("1. View Investment Categories\n");
        (void)printf("2. Get Recommendation Based on Amount\n");
        (void)printf("3. Back to Main Menu\n");
        (void)printf("Enter Choice: ");

        if(readValidInt(&choice) == 0)
        {
            /* invalid entry - re-prompt */
        }
        else
        {
            switch(choice)
            {
                case 1:
                    viewInvestmentCategories();
                    break;

                case 2:
                    getInvestmentRecommendation();
                    break;

                case 3:
                    done = 1;
                    break;

                default:
                    (void)printf("Invalid Choice\n");
                    break;
            }
        }
    }
}
