// atoff.c - CMOC's standard library functions.
//
// By Pierre Sarrazin <http://sarrazip.com/>.
// This file is in the public domain.

#include "cmoc.h"


#ifdef _COCO_BASIC_

float atoff(_CMOC_CONST_ char *nptr)
{
    char *endptr;
    return strtof(nptr, &endptr);
}

#endif  /* _COCO_BASIC_ */
