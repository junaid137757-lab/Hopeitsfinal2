#ifndef PERSISTENCE_H
#define PERSISTENCE_H

/* Data Storage & Persistence module: reads/writes the
   three per-user .dat files (transactions, budgets, goals). */

void loadTransactions(void);
void saveTransactions(void);

void loadBudgets(void);
void saveBudgets(void);

void loadGoals(void);
void saveGoals(void);

void loadEmergencyFund(void);
void saveEmergencyFund(void);

#endif
