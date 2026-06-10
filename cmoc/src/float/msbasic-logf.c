#include "cmoc-stdlib-private.h"


float
logf(float x)
{
    _CMOC_applyRealFunction((void *) _CMOC_ecb_or_dgn(0x8446, 0x923C), &x);
    return x;
}
