// mc6839-log2f.c - Binary logarithm using the MC6839 ROM image.

#include "mc6839-cmoc.h"


float
log2f(float x)
{
    if (x == 0)
        return * (float *) _CMOC_MINUS_HUGE_VALF;
    if (x < 0)
        return * (float *) _CMOC_NAN;
    return _CMOC_log2fOfPosValue(x);
}
