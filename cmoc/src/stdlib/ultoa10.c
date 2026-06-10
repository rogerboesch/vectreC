// ultoa10.c - CMOC's standard library functions.
//
// By Pierre Sarrazin <http://sarrazip.com/>.
// This file is in the public domain.

#include <cmoc.h>


char *_FinishIntegerToASCII(char *firstDigit, char *endOfString);


static const unsigned long ten32Bits = 10UL;


char *
ultoa10(unsigned long value, char *str)
{
    if (value == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }

    char *writer = str;

    // Write the digits in reverse order, then reverse them.

    #if 0  /* Original C loop. */
    while (value > 0)
    {
        unsigned long next = value / 10u;
        *writer++ = '0' + (char) (value - next * 10u);  // value % 10
        value = next;
    }
    #else
    unsigned long results[2];  // [0] will receive quotient, [1] remainer
    asm
    {
DIV32   IMPORT
        bra     @whileValueNotZero
@whileBody
        pshs    y,u             ; save CMOC globals pointer and frame pointer
        leax    :value          ; dividend
        leay    :results        ; 8-byte destination buffer
        leau    :ten32Bits
; Cannot refer to locals now that U is changed.
        lbsr    DIV32           ; divide *X by *U
; 0,Y = quotient
; 4,Y = remainder (0..9), i.e., 7,Y = 0..9 b/c 6809 is big endian
        leax    ,y              ; stop using Y
        puls    u,y             ; restore CMOC pointers: can refer to globals and locals again
; Copy quotient to 'value'.
        ldd     ,x
        std     :value
        ldd     2,x
        std     :value[2]
; Write digit to 'writer'.
        ldb     7,x             ; remainder (0..9)
        addb    #'0'            ; convert to ASCII digit
        ldx     :writer
        stb     ,x+
        stx     :writer
@whileValueNotZero
        ldd     :value          ; check MSB of 'value'
        bne     @whileBody
        ldd     :value[2]       ; check LSB of 'value'
        bne     @whileBody
    }
    #endif
    
    return _FinishIntegerToASCII(str, writer);
}
