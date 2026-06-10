#include <cmoc.h>


size_t
strcspn(const char *s, const char *reject)
{
    size_t counter = 0;
    while (*s != '\0')
    {
        if (strchr(reject, *s) != NULL)
            return counter;
        ++counter;
        ++s;
    }
    return counter;
}
