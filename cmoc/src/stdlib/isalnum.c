#include <cmoc.h>


int
isalnum(int c)
{
    return (c >= '0' && c <= '9') || isalpha(c);
}
