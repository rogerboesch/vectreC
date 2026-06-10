#include <cmoc.h>


size_t
strspn(const char *s, const char *accept)
{
    size_t counter = 0;
    while (*s != '\0' && strchr(accept, *s++) != NULL)
        ++counter;
    return counter;    
}
