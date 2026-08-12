#ifndef UTILITY_H
#define UTILITY_H

void getCurrentDate(char *buffer);

/* Discards any leftover characters (including a bad/partial
   entry) up to the next newline, so a failed or partial scanf
   can never cascade into the next prompt. Call this right
   after every scanf() in the project. */
void clearInputBuffer(void);

/* Reads a full line (spaces allowed) into buffer, up to size-1
   characters, and strips the trailing newline. Use this instead
   of scanf("%s", ...) for any text field that may contain spaces
   (names, categories, goal titles) so input isn't silently cut
   off at the first space. */
void readLine(char *buffer, int size);

/* Reads an int/float safely: returns 1 and stores the value on
   success. On non-numeric input (e.g. typing text where a number
   was expected), prints an error, clears the leftover bad input,
   and returns 0 instead of leaving *value undefined or letting
   garbage data silently pass through. Callers should treat a 0
   return the same as if the user made no valid entry at all. */
int readValidInt(int *value);
int readValidFloat(float *value);

/* Case-insensitive substring check - used for "did you mean"
   style search suggestions. Returns 1 if needle appears anywhere
   in haystack, ignoring case, 0 otherwise. */
int caseInsensitiveContains(const char *haystack, const char *needle);

#endif
