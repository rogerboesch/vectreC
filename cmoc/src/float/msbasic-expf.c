#include "cmoc-stdlib-private.h"


float
expf(float x)
{
    _CMOC_applyRealFunction((void *) _CMOC_ecb_or_dgn(0x84F2, 0x9713), &x);
    return x;
}
