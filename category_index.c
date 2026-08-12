#include <string.h>

#include "category_index.h"
#include "common.h"

/* Table size: a fixed multiple of MAX_BUDGETS so the load
   factor stays well under 100%, keeping average probe
   chains short even when every budget slot is in use.
   Still a compile-time constant - no malloc. */
#define CATEGORY_INDEX_CAPACITY (((unsigned int)MAX_BUDGETS) * 2U)

/* Each slot holds either -1 (empty) or a valid index into
   budgets[]. There is no separate chaining structure - on
   a collision we probe the next slot, wrapping around,
   until we find an empty one or the matching category. */
static int slots[CATEGORY_INDEX_CAPACITY];

/* djb2 string hash. Not cryptographic - just needs to
   spread category names evenly across the table so probe
   chains stay short. */
static unsigned int hashCategory(const char *category)
{
    unsigned int hash = 5381u;
    const char *p = category;

    while(*p != '\0')
    {
        hash = ((hash << 5) + hash) + (unsigned int)(unsigned char)(*p);
        p++;
    }

    return hash;
}

void categoryIndexRebuild(void)
{
    unsigned int i;

    for(i = 0u; i < CATEGORY_INDEX_CAPACITY; i++)
    {
        slots[i] = -1;
    }

    for(i = 0u; i < (unsigned int)budgetCount; i++)
    {
        unsigned int h = hashCategory(budgets[i].category) % CATEGORY_INDEX_CAPACITY;

        while(slots[h] != -1)
        {
            h = (h + 1u) % CATEGORY_INDEX_CAPACITY;
        }

        slots[h] = (int)i;
    }
}

int categoryIndexLookup(const char *category)
{
    unsigned int h = hashCategory(category) % CATEGORY_INDEX_CAPACITY;
    unsigned int probes = 0u;
    int result = -1;
    int done = 0;

    while((done == 0) && (slots[h] != -1) && (probes < CATEGORY_INDEX_CAPACITY))
    {
        if(strcmp(budgets[slots[h]].category, category) == 0)
        {
            result = slots[h];
            done = 1;
        }
        else
        {
            h = (h + 1u) % CATEGORY_INDEX_CAPACITY;
            probes++;
        }
    }

    return result;
}
