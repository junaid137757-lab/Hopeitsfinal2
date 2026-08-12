#ifndef BUDGET_H
#define BUDGET_H

void setBudget(void);
void viewBudgets(void);
void checkBudgetAlert(const char *category);

/* Exposed so Dashboard and Recommendation modules can
   read live spend-per-category without duplicating logic. */
float getSpentForCategory(const char *category);

/* Binary search over budgets[], which setBudget() keeps
   sorted by category at all times. Returns the matching
   index, or -1 if no budget exists for that category.
   O(log n) versus the O(n) linear scan it replaces. */
int findBudgetIndex(const char *category);

/* Re-sorts budgets[] by category using qsort(). setBudget()
   maintains this invariant incrementally on every insert,
   but persistence.c calls this defensively right after
   loading from disk, in case an on-disk file predates this
   invariant or was written by another tool. */
void sortBudgetsByCategory(void);

#endif
