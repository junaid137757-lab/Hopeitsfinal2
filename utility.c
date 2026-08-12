#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "utility.h"

void getCurrentDate(char *buffer)
{
    time_t t = time(NULL);
    const struct tm *tm_info = localtime(&t);

    (void)strftime(buffer, 11U, "%Y-%m-%d", tm_info);
}

void clearInputBuffer(void)
{
    int c;
    int done = 0;

    while(done == 0)
    {
        c = getchar();

        if((c == (int)'\n') || (c == EOF))
        {
            done = 1;
        }
    }
}

void readLine(char *buffer, int size)
{
    if(fgets(buffer, size, stdin) != NULL)
    {
        size_t len = strlen(buffer);

        if((len > 0U) && (buffer[len - 1U] == '\n'))
        {
            buffer[len - 1U] = '\0';
        }
        else
        {
            /* Input was longer than the buffer - discard the rest
               of the line so it doesn't spill into the next prompt. */
            clearInputBuffer();
        }
    }
    else
    {
        buffer[0] = '\0';
    }
}

int readValidInt(int *value)
{
    int result;

    if(scanf("%d", value) != 1)
    {
        clearInputBuffer();
        (void)printf("Invalid input - please enter a whole number.\n");
        result = 0;
    }
    else
    {
        clearInputBuffer();
        result = 1;
    }

    return result;
}

int readValidFloat(float *value)
{
    int result;

    if(scanf("%f", value) != 1)
    {
        clearInputBuffer();
        (void)printf("Invalid input - please enter a number.\n");
        result = 0;
    }
    else
    {
        clearInputBuffer();
        result = 1;
    }

    return result;
}

int caseInsensitiveContains(const char *haystack, const char *needle)
{
    int result = 0;
    int done = 0;
    const char *outer = haystack;

    if(*needle == '\0')
    {
        result = 1;
        done = 1;
    }

    while((done == 0) && (*outer != '\0'))
    {
        const char *h = outer;
        const char *n = needle;
        int mismatch = 0;

        while((*h != '\0') && (*n != '\0') && (mismatch == 0))
        {
            if(tolower((unsigned char)*h) != tolower((unsigned char)*n))
            {
                mismatch = 1;
            }
            else
            {
                h++;
                n++;
            }
        }

        if((mismatch == 0) && (*n == '\0'))
        {
            result = 1;
            done = 1;
        }
        else
        {
            outer++;
        }
    }

    return result;
}
