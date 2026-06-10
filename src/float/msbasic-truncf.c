#include "cmoc-stdlib-private.h"


float
truncf(float x)
{
    _CMOC_applyRealFunction((void *) _CMOC_ecb_or_dgn(0x8524, 0x9956), &x);  // call Basic's FIX function
    return x;
}
