// atoul16.c - CMOC's standard library functions.
//
// By Pierre Sarrazin <http://sarrazip.com/>.
// This file is in the public domain.

#include "cmoc.h"


unsigned long atoul16(_CMOC_CONST_ char *nptr)
{
    char *endptr;
    return strtoul16(nptr, &endptr);
}
