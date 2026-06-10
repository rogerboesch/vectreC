#include "cmoc-stdlib-private.h"


float
cosf(float radians)
{
    _CMOC_applyRealFunction((void *) _CMOC_ecb_or_dgn(0x8378, 0x97CB), &radians);
    return radians;
}
