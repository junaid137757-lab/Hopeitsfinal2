#ifndef NOTIFICATION_H
#define NOTIFICATION_H

/* Notification & Alerts module.
   Other modules (budget, savings goal) call into this
   instead of printing alert text themselves, so all
   user-facing alert wording lives in one place. */

void notifyBudgetStatus(const char *category, float spent, float limit);
void notifyGoalAchieved(const char *goalName);

#endif
