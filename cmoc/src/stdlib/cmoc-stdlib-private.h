/*  cmoc-stdlib-private.h

    By Pierre Sarrazin <http://sarrazip.com/>.
    This file is in the public domain.
*/

#ifndef _H_cmoc_stdlib_private
#define _H_cmoc_stdlib_private

#include <coco.h>


void _CMOC_applyRealFunction(void *fpa0Transform, float *p);


#if defined(_COCO_BASIC_)
#define _CMOC_ecb_or_dgn(ecb_value, dgn_value) ecb_value
#elif defined(DRAGON)
#define _CMOC_ecb_or_dgn(ecb_value, dgn_value) dgn_value
#endif


enum
{
    MAX_NUM_GRANULES = 68,
    GRANULE_SIZE = 2304,
};


typedef interrupt void (*_CMOC_ISR)(void);


typedef byte (*_CMOC_ReadDiskSectorFuncPtr)(byte dest[256],
                                            byte drive, byte track, byte sector);


struct _CMOC_FileDesc
{
    byte driveNo;  // DECB drive number
    byte curGran;  // 0..MAX_NUM_GRANULES-1, 255 means at EOF
    byte curSec;  // 1..9 (relative to current granule)
    _CMOC_ReadDiskSectorFuncPtr readDiskSectorFuncPtr;
};


byte _CMOC_readDECBFile2(void *dest,
                         byte driveNo, const char filename[11],
                         byte workBuffer[256], size_t *sizePtr,
                         _CMOC_ReadDiskSectorFuncPtr readDiskSectorFuncPtr);


#endif  /* _H_cmoc_stdlib_private */
