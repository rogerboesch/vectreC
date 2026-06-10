/*  readDECBFileWithDECB.c - DECB-type file reading function that does not depend on DECB routines.

    By Pierre Sarrazin <http://sarrazip.com/>.
    This file is in the public domain.
*/

#include "cmoc-stdlib-private.h"


static byte
readDiskSectorWithDECB(byte dest[256], byte drive, byte track, byte sector)
{
    byte *vars = * (byte **) 0xC006;  // address of Disk Basic's DSKCON variables
    vars[0] = 2;  // read
    vars[1] = drive;
    vars[2] = track;
    vars[3] = sector;
    * (byte **) (vars + 4) = dest;
    asm
    {
        pshs    u           ; preserve CMOC frame pointer;
                            ; Y not preserved b/c not used by CMOC under DECB
        jsr     [$C004]     ; invoke Disk Basic DSKCON
        puls    u
    }
    return vars[6] == 0;
}


byte
readDECBFileWithDECB(void *dest,
                     byte driveNo, const char filename[11],
                     byte workBuffer[256], size_t *sizePtr)
{
    return _CMOC_readDECBFile2(dest, driveNo, filename,
                               workBuffer, sizePtr, readDiskSectorWithDECB);
}
