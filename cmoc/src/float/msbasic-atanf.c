#include "cmoc-stdlib-private.h"


float
atanf(float tangent)
{
    _CMOC_applyRealFunction((void *) _CMOC_ecb_or_dgn(0x83B0, 0x9877), &tangent);
    return tangent;  // angle in radians
}
