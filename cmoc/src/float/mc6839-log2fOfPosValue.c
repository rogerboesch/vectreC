// mc6839-log2fOfPosValue.c - Utility function to compute the binary logarithm using the MC6839 ROM image.

#include "mc6839-cmoc.h"


//#define USE_ONLY_C


// Assumes x is a big-endian 32-bit IEEE 754 float.
// Such a float has a 23-bit mantissa and an exponent bias of 127.
// Details on the algorithm:
//   https://en.wikipedia.org/wiki/Binary_logarithm#Iterative_approximation
//
float
_CMOC_log2fOfPosValue(float x)
{
    signed char unbiasedExp;
    unsigned long result;

    #ifdef USE_ONLY_C

    // x is 2**k * 1.mantissa,
    // so log2(x) = log2(2**k) + log2(1.mantissa)
    //            = k + log2(1.mantissa).
    // Compute k by extracting the exponent in x.
    //
    unbiasedExp = ((signed char) (* (unsigned short *) &x >> 7)) - 127;

    // Compute log2(1.mantissa) using an algorithm that assumes that
    // the argument is in [1, 2). Source: Knuth I.
    //
    // Set exponent of x to 0, so that x becomes 2**0 * 1.mantissa, i.e., 1.mantissa.
    ((unsigned char *) &x)[0] = 0x3f;
    ((unsigned char *) &x)[1] |= 0x80;

    result = 0;
    for (unsigned char i = 0; i < 23; ++i)
    {
        result <<= 1;
        x *= x;
        if (x >= 2)  // if higher than the base of the logarithm
        {
            result |= 1;
            x /= 2;
        }
    }

    #else  /* ndef USE_ONLY_C: */

    unsigned char i;
    asm
    {
mulSingleSingle IMPORT
FPCB_SINGLE     IMPORT
MC6839_ROM      IMPORT
        ;
        ; unbiasedExp = ((signed char) (* (unsigned short *) &x >> 7)) - 127;
        ;
        ldd     :x              ; bits 14..7 contain biased exponent
        lslb
        rola                    ; A now contains biased exponent
        suba    #127            ; remove exponent bias
        sta     :unbiasedExp
        ;
        ; Set exponent of x to 0.
        ;
        ldb     #$3F
        stb     :x
        ldb     :x[1]
        orb     #$80
        stb     :x[1]
        ;
        ; result = 0;
        ;
        clra
        clrb
        std     :result
        std     :result[2]
        ;
        ; Start loop on i.
        ;
        ldb     #23                 ; size of mantissa of float
        stb     :i
        ;
@loop
        ; result <<= 1;
        ;
        lsl     :result[3]
        rol     :result[2]
        rol     :result[1]
        rol     :result[0]
        ;
        ; x *= x;
        ;
        ; Call the FMUL operation of the 6839 ROM.
        pshs    u,y
        leax    FPCB_SINGLE,pcr
        tfr     x,d                     ; point D to single-precision type
        leax    :x                      ; point X to result number
        leau    ,x                      ; point U to 1st number
        leay    ,x                      ; point Y to 2nd number
        lbsr    MC6839_ROM+$3D
        fcb     $04                     ; FMUL
        puls    y,u
        ;
        ; if (x >= 2)
        ; i.e., check if exp. is >= 1, i.e., if biased exp. is >= 128.
        ;
        ldd     :x                  ; get high word
        lslb
        rola                        ; biased exp now in A
        cmpa    #128
        blo     @lowerThan2
        ;
        ; result |= 1;
        ;
        ldb     :result[3]
        orb     #1
        stb     :result[3]
        ;
        ; x /= 2;
        ;
        ldd     :x                  ; get high word
        subd    #$80                ; decrement biased exp where it is in this word
        std     :x
@lowerThan2
        dec     :i
        bne     @loop
    }

    #endif  /* ndef USE_ONLY_C */

    return unbiasedExp + (float) result / 0x800000ul;  // divisor is 2**23
}

