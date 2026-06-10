#include <cmoc.h>


void *
memchr(const void *s, int c, size_t n)
{
    void *ret;
    asm
    {
        pshs    y
        ldy     :n          ; cannot refer to globals until Y is restored
        beq     @notFound   ; if no bytes to look at
        ldx     :s
@loop
        lda     ,x+
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
    }
    return ret;
}

