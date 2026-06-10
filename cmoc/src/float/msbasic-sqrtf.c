#include "cmoc-stdlib-private.h"


float
sqrtf(float x)
{
    _CMOC_applyRealFunction((void *) _CMOC_ecb_or_dgn(0x8480, 0x9697), &x);
    return x;
}
