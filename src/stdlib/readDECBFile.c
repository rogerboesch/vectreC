/*  readDECBFile.c - DECB-type file reading function that does not depend on DECB routines.

    By Pierre Sarrazin <http://sarrazip.com/>.
    This file is in the public domain.
*/

#include "cmoc-stdlib-private.h"
#include "dskcon-standalone.h"


static asm void
copy3Bytes(void *dest, const void *src)
{
#ifdef OS9
#error "OS-9 not supported"
#endif
    asm
    {
        ldx     4,s     ; src
        ldy     2,s     ; dest (Y not preserved b/c CMOC does not use it for DECB)
        lda     ,x+
        sta     ,y+
        ldd     ,x
        std     ,y
    }
}


static void
readDECBFile_setISR(void *vector, _CMOC_ISR newRoutine, byte originalBytes[3])
{
    byte *isr = * (byte **) vector;
    copy3Bytes(originalBytes, isr);
    *isr = 0x7E;  // JMP extended
    * (_CMOC_ISR *) (isr + 1) = newRoutine;
}


#define unsetISR(vector, originalBytes) (copy3Bytes(* (byte **) (vector), (originalBytes)))


static interrupt asm void
readDECBFile_irqService(void)
{
    asm
    {
_dskcon_irqService IMPORT
        ldb     $FF03
        bpl     @done               // do nothing if 63.5 us interrupt
        ldb     $FF02               // 60 Hz interrupt. Reset PIA0, port B interrupt flag.
        lbsr    dskcon_irqService
@done
    }
}


static byte
readDiskSectorStandalone(byte dest[256], byte drive, byte track, byte sector)
{
    DCOPC = 2;  // read
    DCDRV = drive;
    DCTRK = track;
    DCSEC = sector;
    DCBPT = dest;
    dskcon_processSector();
    return DCSTA == 0;
}


byte
readDECBFile(void *dest,
             byte driveNo, const char filename[11],
             byte workBuffer[256], size_t *sizePtr)
{
    byte originalIRQISRBytes[3];

    // Set up an IRQ service routine (dskcon_nmiService()) that allows
    // using the standalone sector I/O routine (dskcon_processSector()).
    //
    disableInterrupts();
    readDECBFile_setISR(0xFFF8, readDECBFile_irqService, originalIRQISRBytes);
    const unsigned long cookie = dskcon_init(dskcon_nmiService);
    enableInterrupts();  // IRQ is needed during disk operations

    byte err = _CMOC_readDECBFile2(dest, driveNo, filename,
                                   workBuffer, sizePtr, readDiskSectorStandalone);

    // Interrupts are masked at this point.

    dskcon_shutdown(cookie);
    unsetISR(0xFFF8, originalIRQISRBytes);

    return err;
}


#if 0  /* Self-test. */

int
main()
{
    char *dest = 0x6000;
    printf("LOADING FILE TO %p.\n", dest);

    byte workBuffer[256];
    size_t numBytesRead;
    byte err = readDECBFile(dest, 0, "TEST    TXT", workBuffer, &numBytesRead);
    if (err != 0)
    {
        printf("ERROR #%u\n", err);
        return 1;
    }

    printf("READ %u BYTES(S).\n", numBytesRead);
    if (numBytesRead > 0)
        for (size_t offset = 0; offset < numBytesRead; offset += 128)
        {
            putstr(dest + offset, 128);
            waitkey(TRUE);
        }
    printf("\n""PASSED. TYPE DIR TO CHECK THAT\nDISK BASIC STILL RUNS FINE.\n");
    return 0;
}

#endif
