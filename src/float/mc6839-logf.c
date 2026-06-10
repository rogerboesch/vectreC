// mc6839-logf.c - Natural logarithm using the MC6839 ROM image.

#include "mc6839-cmoc.h"


static const float log2OfE = 1.44269504089f;  // log2(e), i.e., log2(2.718281828...)


float
logf(float x)
{
    if (x == 0)
        return * (float *) _CMOC_MINUS_HUGE_VALF;
    if (x < 0)
        return * (float *) _CMOC_NAN;
    return _CMOC_log2fOfPosValue(x) / log2OfE;    
}
