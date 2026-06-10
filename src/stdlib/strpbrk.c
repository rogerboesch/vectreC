#include <cmoc.h>


char *
strpbrk(const char *s, const char *accept)
{
    while (*s != '\0')
    {
        if (strchr(accept, *s) != NULL)
            return (char *) s;
        ++s;
    }
    return NULL;
}
