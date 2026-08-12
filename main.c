#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "utility.h"
#include "authentication.h"
#include "persistence.h"
#include "income.h"
#include "expense.h"
#include "transaction.h"
#include "budget.h"
#include "savings_goal.h"
#include "dashboard.h"
#include "reports.h"
#include "recommendation.h"
#include "investment.h"
#include "activity_log.h"
#include "logger.h"
#include "autosave.h"

/* MISRA-C:2012 note: this file previously used goto to break out
   of a nested menu loop and called exit() from inside a switch
   case (Rule 15.1 discourages goto; Rule 21.8 disallows the
   stdlib exit()/abort() family). Both are gone: the logged-in
   menu is its own function that loops on a `loggedOut` flag, and
   main() loops on a `running` flag with a single `return` at the
   end - one point of entry, one point of exit, no goto. */

/* Runs the post-login menu until the user chooses "Logout".
   Loading the user's data, starting the autosave thread, and
   resetting the activity log all happen once here, right after
   a successful login. */
static void runLoggedInMenu(void)
{
    int option;
    int loggedOut = 0;
    char logMsg[80];

    loadTransactions();
    loadBudgets();
    loadGoals();
    loadEmergencyFund();

    (void)startAutosaveThread();

    /* The activity log is one shared in-memory buffer for the
       whole running process (see activity_log.c) - without this
       reset, option 19 would keep showing every previous user's
       history from this same run of the app, which is a real
       privacy problem, not just clutter. Resetting here means
       each login starts a clean, this-session-only activity
       feed. */
    resetActivityLog();

    (void)snprintf(logMsg, sizeof(logMsg), "Logged in as %s", currentUser);
    logActivity(logMsg);

    while(loggedOut == 0)
    {
        (void)printf("\n=====================================\n");
        (void)printf("Welcome %s\n", currentUser);
        (void)printf("=====================================\n");

        (void)printf(" 1. Add Income\n");
        (void)printf(" 2. Add Expense\n");
        (void)printf(" 3. View Transactions\n");
        (void)printf(" 4. Edit Transaction\n");
        (void)printf(" 5. Delete Transaction\n");
        (void)printf(" 6. Search Transactions by Category\n");
        (void)printf(" 7. Set Budget\n");
        (void)printf(" 8. View Budgets\n");
        (void)printf(" 9. Set Savings Goal\n");
        (void)printf("10. Contribute to Savings Goal\n");
        (void)printf("11. View Savings Goals\n");
        (void)printf("12. Financial Dashboard\n");
        (void)printf("13. Category-wise Report\n");
        (void)printf("14. Monthly Report\n");
        (void)printf("15. Smart Recommendations\n");
        (void)printf("16. Investment\n");
        (void)printf("17. Change Password\n");
        (void)printf("18. Logout\n");
        (void)printf("19. Recent Activity\n");

        (void)printf("Enter Choice: ");

        if(readValidInt(&option) == 0)
        {
            /* invalid entry - re-prompt */
        }
        else
        {
            switch(option)
            {
                case 1:
                    addIncome();
                    break;

                case 2:
                    addExpense();
                    break;

                case 3:
                    viewTransactions();
                    break;

                case 4:
                    editTransaction();
                    break;

                case 5:
                    deleteTransaction();
                    break;

                case 6:
                    searchTransactionsByCategory();
                    break;

                case 7:
                    setBudget();
                    break;

                case 8:
                    viewBudgets();
                    break;

                case 9:
                    setSavingsGoal();
                    break;

                case 10:
                    contributeToGoal();
                    break;

                case 11:
                    viewSavingsGoals();
                    break;

                case 12:
                    financialDashboard();
                    break;

                case 13:
                    categoryWiseReport();
                    break;

                case 14:
                    monthlyReport();
                    break;

                case 15:
                    generateRecommendations();
                    break;

                case 16:
                    investmentMenu();
                    break;

                case 17:
                    changePassword();
                    break;

                case 18:
                    stopAutosaveThread();
                    saveTransactions();
                    saveBudgets();
                    saveGoals();
                    saveEmergencyFund();
                    (void)printf("Logged Out Successfully!\n");
                    loggedOut = 1;
                    break;

                case 19:
                    printActivityLog();
                    break;

                default:
                    (void)printf("Invalid Choice\n");
                    break;
            }
        }
    }
}

int main(void)
{
    int choice;
    int running = 1;

    /* Force output to appear immediately, rather than sitting in a
       buffer until it happens to flush. Without this, some terminals
       (notably certain VS Code setups) can delay a prompt's text
       until after the next read, making it look like a field was
       skipped even though it wasn't. */
    (void)setvbuf(stdout, NULL, _IONBF, 0);

    if(logInit("data/app.log") == 0)
    {
        (void)printf("Warning: could not open log file - continuing without file logging.\n");
    }

    while(running != 0)
    {
        (void)printf("\n=====================================\n");
        (void)printf(" DIGITAL PERSONAL FINANCE PLATFORM\n");
        (void)printf("=====================================\n");

        (void)printf("1. Register\n");
        (void)printf("2. Login\n");
        (void)printf("3. Forgot Password\n");
        (void)printf("4. Exit\n");

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
                    registerUser();
                    break;

                case 2:
                {
                    int result = loginUser();

                    if(result == 1)
                    {
                        runLoggedInMenu();
                    }

                    break;
                }

                case 3:
                    forgotPassword();
                    break;

                case 4:
                    (void)printf("Thank You!\n");
                    stopAutosaveThread();
                    LOG_INFO_MSG("user exited application normally");
                    logClose();
                    running = 0;
                    break;

                default:
                    (void)printf("Invalid Choice\n");
                    break;
            }
        }
    }

    return 0;
}
