#!/bin/bash
# Builds the interactive Digital Personal Finance Platform app.
# Excludes benchmark.c and every test_*.c file - each has its own
# main(), and would collide with main.c if compiled together.
set -e
mkdir -p data
gcc main.c globals.c utility.c notification.c authentication.c persistence.c \
    income.c expense.c transaction.c budget.c savings_goal.c dashboard.c \
    reports.c recommendation.c investment.c category_index.c activity_log.c \
    logger.c autosave.c sha256.c \
    -Wall -Wextra -pthread -o finance
echo "Built ./finance"
