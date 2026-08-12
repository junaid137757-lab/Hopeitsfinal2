#include <stdio.h>
#include <string.h>

#include "activity_log.h"

/* =====================================================
   A fixed-size pool of nodes doubles as TWO singly-linked
   lists, both threaded through the same array via integer
   indices (instead of pointers, since there's no heap):

     head/tail : the IN-USE list, most-recent-first,
                 `head` is the newest entry, `tail` the
                 oldest (and the first to be reclaimed)
     freeHead  : the FREE list - unused slots ready to be
                 handed out to the next logActivity() call

   Adding an entry pops a node off the free list. Once the
   free list is empty (the log is full), the oldest in-use
   node (tail) is reclaimed and reused instead - so this
   behaves like a bounded ring buffer that never grows past
   ACTIVITY_LOG_CAPACITY and never calls malloc/free.
   ===================================================== */

typedef struct
{
    char message[80];
    int next; /* index of the next node in whichever list
                 this node currently belongs to, or -1 for
                 "end of list" */
} LogNode;

static LogNode pool[ACTIVITY_LOG_CAPACITY];
static int head = -1;
static int tail = -1;
static int freeHead = 0;
static int initialized = 0;

static void initPoolIfNeeded(void)
{
    int i;

    if(initialized == 0)
    {
        for(i = 0; i < (ACTIVITY_LOG_CAPACITY - 1); i++)
        {
            pool[i].next = i + 1;
        }

        pool[ACTIVITY_LOG_CAPACITY - 1].next = -1;

        freeHead = 0;
        head = -1;
        tail = -1;
        initialized = 1;
    }
}

void resetActivityLog(void)
{
    initialized = 0;
    initPoolIfNeeded();
}

void logActivity(const char *message)
{
    int node;

    initPoolIfNeeded();

    if(freeHead != -1)
    {
        /* Common case: pop a node off the free list. */
        node = freeHead;
        freeHead = pool[node].next;
    }
    else
    {
        /* Pool is full - reclaim the oldest in-use node
           (tail) instead of dropping the new entry. */
        node = tail;

        if(head == tail)
        {
            head = -1;
            tail = -1;
        }
        else
        {
            /* Singly-linked, so finding tail's predecessor
               means walking from head - fine at this size
               (ACTIVITY_LOG_CAPACITY is small). */
            int walker = head;

            while(pool[walker].next != tail)
            {
                walker = pool[walker].next;
            }

            pool[walker].next = -1;
            tail = walker;
        }
    }

    (void)strncpy(pool[node].message, message, sizeof(pool[node].message) - 1U);
    pool[node].message[sizeof(pool[node].message) - 1U] = '\0';

    pool[node].next = head;
    head = node;

    if(tail == -1)
    {
        tail = node;
    }
}

void printActivityLog(void)
{
    int cursor;

    initPoolIfNeeded();

    if(head == -1)
    {
        (void)printf("No Activity Logged Yet\n");
    }
    else
    {
        (void)printf("\n--------------------------------------------------------------------\n");
        (void)printf("RECENT ACTIVITY (most recent first)\n");
        (void)printf("--------------------------------------------------------------------\n");

        cursor = head;

        while(cursor != -1)
        {
            (void)printf("- %s\n", pool[cursor].message);
            cursor = pool[cursor].next;
        }
    }
}
