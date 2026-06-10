// _FinishIntegerToASCII.c - CMOC's standard library functions.
//
// By Pierre Sarrazin <http://sarrazip.com/>.
// This file is in the public domain.

#include <cmoc.h>


// Invert characters from 'firstDigit' to before 'endOfString'.
// Returns 'firstDigit' as is.
//
char *
_FinishIntegerToASCII(char *firstDigit, char *endOfString)
{
    #if 0  /* Original C code. */

    char *retval = firstDigit;
    *endOfString = '\0';
    for (--endOfString; firstDigit < endOfString; ++firstDigit, --endOfString)
    {
        char tmp = *firstDigit;
        *firstDigit = *endOfString;
        *endOfString = tmp;
    }
    return retval;

    #else

    asm   // This routine must not change 'firstDigit'.
    {
        pshs    y                   ; preserve CMOC globals pointer
        ldx     :firstDigit
        ldy     :endOfString        ; cannot refer to globals now
        clr     ,y
        leay    -1,y
        bra     @loopCond
@loopBody
; Swap ,x and ,y.
        lda     ,x
        ldb     ,y
        sta     ,y
        stb     ,x+
        leay    -1,y                ; --endOfString
@loopCond
        sty     :endOfString        ; store for comparison
        cmpx    :endOfString
        blo     @loopBody
        puls    y
    }

    return firstDigit;
    #endif
}
