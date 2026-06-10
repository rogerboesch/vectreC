#include "cmoc-stdlib-private.h"


float
floorf(float x)
{
    _CMOC_applyRealFunction((void *) _CMOC_ecb_or_dgn(0xBCEE, 0x9499), &x);  // call Basic's INT function
    return x;
}
