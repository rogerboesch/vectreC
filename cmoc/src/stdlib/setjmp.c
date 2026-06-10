/*  setjmp.c - Nonlocal goto.

    By Pierre Sarrazin <http://sarrazip.com/>.
    This file is in the public domain.
*/

#include "setjmp.h"


asm int setjmp(jmp_buf env)
{
    asm
    {
        ldx     2,s     ; point to 'env', a 6-byte array
        ldd     ,s      ; return address, to be jumped to by longjmp()
        std     ,x

        tfr     s,d
        addd    #2      ; value of S before the LBSR/JSR to setjmp() (one word past the current return address)
        std     2,x

        stu     4,x     ; frame pointer

        clra            ; return 0 in D
        clrb
    }
}


asm __norts__ void
longjmp(jmp_buf env, int value)
{
    asm
    {
        ldd     2+2,s   ; load 'value' before S points to original stack position
        bne     @gotValue
        incb            ; caller mistakenly passed 0 for value; return 1 (in D) instead
@gotValue
        ldx     2,s     ; point to 'env'
        lds     2,x
        ldu     4,x
        jmp     [,x]    ; return to caller of setjmp() with 'value' in D
    }
}
