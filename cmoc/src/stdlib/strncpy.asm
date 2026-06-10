        SECTION code

_strncpy        EXPORT


* char *strncpy(char *dest, const char *src, size_t n);
* Returns dest.
*
_strncpy
        pshs    u,y
        ldx     6,s             dest
        ldu     8,s             src
        ldy     #0              counts bytes filled
        bra     @copyLoopCond
@copyLoop
        lda     ,u+             is next source byte terminator?
        beq     @fillLoopCond   yes
        sta     ,x+             store in destination string
        leay    1,y             one more byte filled
@copyLoopCond
        cmpy    10,s            filled all bytes to fill?
        blo     @copyLoop       no
        bra     @done
@fillLoop
        clr     ,x+             pad with '\0'
        leay    1,y
@fillLoopCond
        cmpy    10,s            filled all bytes to fill?
        blo     @fillLoop       no
@done
        ldd     6,s             dest
        puls    y,u,pc


        ENDSECTION
