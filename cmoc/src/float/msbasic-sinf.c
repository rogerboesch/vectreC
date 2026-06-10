#include "cmoc-stdlib-private.h"


float
sinf(float radians)
{
    _CMOC_applyRealFunction((void *) _CMOC_ecb_or_dgn(0xBF78, 0x97D1), &radians);
    return radians;
}
