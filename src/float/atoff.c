// atoff.c - CMOC's standard library functions.
//
// By Pierre Sarrazin <http://sarrazip.com/>.
// This file is in the public domain.

#include "cmoc.h"


float atoff(_CMOC_CONST_ char *nptr)
{
    char *endptr;
    return strtof(nptr, &endptr);
}
