#include <cmoc.h>


char *
strtok(char *str, const char *delim)
{
    static char *currentString = NULL;

    if (str != NULL)
        currentString = str;
    else if (currentString == NULL)
        return NULL;

    str = currentString + strspn(currentString, delim);
    currentString = str + strcspn(str, delim);
    if (currentString == str)
    {
        currentString = NULL;
        return NULL;
    }

    if (*currentString != '\0')
    {
        *currentString = '\0';
        ++currentString;
    }
    else
        currentString = NULL;

    return str;
}
