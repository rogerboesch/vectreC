#include <cmoc.h>


void *
memichr(const void *s, int c, size_t n)
{
    void *ret;
    asm
    {
; Convert 'c' to lower-case.
        lda     :c[1]       ; low byte of 'c' (big endian)
        bsr     Memichr_convertAToLowerCase
        sta     :c[1]
        pshs    y
        ldy     :n          ; cannot refer to globals until Y is restored
        beq     @notFound   ; if no bytes to look at
        ldx     :s
@loop
        lda     ,x+
        bsr     Memichr_convertAToLowerCase
        cmpa    :c[1]       ; low byte of 'c' (big endian)
        beq     @found      ; if found (at X-1)
        leay    -1,y        ; more bytes to look at?
        bne     @loop       ; if yes
@notFound
        ldx     #1          ; return NULL
@found
        leax    -1,x        ; go back to found char
        stx     :ret
        puls    y
        bra     @done
;
Memichr_convertAToLowerCase
        cmpa    #'A'
        blo     @notUpperCase
        cmpa    #'Z'
        bhi     @notUpperCase
        adda    #32
@notUpperCase
        rts
@done
    }
    return ret;
}
