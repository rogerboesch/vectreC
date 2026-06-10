#include "cmoc-stdlib-private.h"


float
tanf(float radians)
{
    _CMOC_applyRealFunction((void *) _CMOC_ecb_or_dgn(0x8381, 0x9816), &radians);
    return radians;
}
