#include "CUnit.h"
#include <stdio.h>
#include <string.h>

#include "test_helpers.h"
#include "category_index.h"
#include "common.h"

static int suite_init(void)
{
    th_reset_globals();
    return 0;
}

static int suite_clean(void)
{
    return 0;
}

/* After rebuilding the index from a populated budgets[], a lookup
   for each category should return its correct budgets[] index. */
static void test_lookup_finds_each_category(void)
{
    th_reset_globals();

    strcpy(budgets[0].category, "Food");
    budgets[0].limit = 100.0f;
    strcpy(budgets[1].category, "Travel");
    budgets[1].limit = 200.0f;
    strcpy(budgets[2].category, "Rent");
    budgets[2].limit = 300.0f;
    budgetCount = 3;

    categoryIndexRebuild();

    CU_ASSERT_EQUAL(categoryIndexLookup("Food"), 0);
    CU_ASSERT_EQUAL(categoryIndexLookup("Travel"), 1);
    CU_ASSERT_EQUAL(categoryIndexLookup("Rent"), 2);
}

/* A category that was never added should return -1, not a false
   match or an out-of-range index. */
static void test_lookup_missing_category(void)
{
    th_reset_globals();

    strcpy(budgets[0].category, "Food");
    budgets[0].limit = 100.0f;
    budgetCount = 1;

    categoryIndexRebuild();

    CU_ASSERT_EQUAL(categoryIndexLookup("Nonexistent"), -1);
}

/* Rebuilding with an empty budgets[] must clear out any entries
   left over from a previous rebuild - this is what keeps the index
   from going stale when a different user (with fewer or no
   budgets) logs in after one with budgets set. */
static void test_rebuild_clears_stale_entries(void)
{
    th_reset_globals();

    strcpy(budgets[0].category, "Food");
    budgetCount = 1;
    categoryIndexRebuild();

    CU_ASSERT_EQUAL(categoryIndexLookup("Food"), 0);

    budgetCount = 0;
    categoryIndexRebuild();

    CU_ASSERT_EQUAL(categoryIndexLookup("Food"), -1);
}

/* Rebuilding after budgets[] has shifted (e.g. a sorted insert
   moved every later entry over by one slot) must reflect the NEW
   positions, not the old ones - this is why the index is fully
   rebuilt rather than incrementally patched. */
static void test_rebuild_reflects_shifted_positions(void)
{
    th_reset_globals();

    strcpy(budgets[0].category, "Food");
    budgetCount = 1;
    categoryIndexRebuild();
    CU_ASSERT_EQUAL(categoryIndexLookup("Food"), 0);

    /* simulate a sorted insertion of "Books" before "Food",
       shifting Food from index 0 to index 1 */
    budgets[1] = budgets[0];
    strcpy(budgets[0].category, "Books");
    budgetCount = 2;
    categoryIndexRebuild();

    CU_ASSERT_EQUAL(categoryIndexLookup("Books"), 0);
    CU_ASSERT_EQUAL(categoryIndexLookup("Food"), 1);
}

/* Enough categories to force at least one hash collision (given
   the table is sized to MAX_BUDGETS * 2, filling it to MAX_BUDGETS
   guarantees a >=50% load factor) should still all be found
   correctly - this exercises the linear-probing collision path. */
static void test_lookup_with_many_categories(void)
{
    int i;
    char names[50][10];

    th_reset_globals();

    for(i = 0; i < 50; i++)
    {
        snprintf(names[i], sizeof(names[i]), "Cat%03d", i);
        strcpy(budgets[i].category, names[i]);
        budgets[i].limit = (float)(i * 10);
    }
    budgetCount = 50;

    categoryIndexRebuild();

    for(i = 0; i < 50; i++)
        CU_ASSERT_EQUAL(categoryIndexLookup(names[i]), i);

    CU_ASSERT_EQUAL(categoryIndexLookup("NotThere"), -1);
}

void add_category_index_suite(void)
{
    CU_pSuite suite = CU_add_suite("category_index", suite_init, suite_clean);

    CU_add_test(suite, "lookup finds each category after rebuild", test_lookup_finds_each_category);
    CU_add_test(suite, "lookup returns -1 for a missing category", test_lookup_missing_category);
    CU_add_test(suite, "rebuild clears stale entries", test_rebuild_clears_stale_entries);
    CU_add_test(suite, "rebuild reflects shifted array positions", test_rebuild_reflects_shifted_positions);
    CU_add_test(suite, "lookup works correctly with many entries (collisions)", test_lookup_with_many_categories);
}
