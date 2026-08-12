#ifndef CATEGORY_INDEX_H
#define CATEGORY_INDEX_H

/* =====================================================
   A small hash index over budgets[], keyed by category
   name, mapping to the matching index in budgets[].

   This is a SECONDARY structure layered on top of the
   budgets[] array - budget.c keeps budgets[] sorted by
   category (for binary search / ordered display), and
   this module keeps a hash table pointing into the same
   array for O(1) average-case point lookups. Both can
   coexist because this index only stores integer array
   positions and gets fully rebuilt whenever those
   positions might have shifted, rather than trying to
   incrementally patch itself - see categoryIndexRebuild().
   ===================================================== */

/* Rebuilds the hash index from the current budgets[] /
   budgetCount. Call this any time budgets[] structurally
   changes - an insert, or a fresh load from disk - since
   the index only stores array positions, and those go
   stale the moment the array is modified. Cheap: O(n)
   with n capped at MAX_BUDGETS. */
void categoryIndexRebuild(void);

/* O(1) average-case lookup: returns the budgets[] index
   for the given category, or -1 if no budget is set for
   it. Backed by a fixed-size open-addressing hash table
   (linear probing) sized to MAX_BUDGETS - no dynamic
   allocation anywhere in this module. */
int categoryIndexLookup(const char *category);

#endif
