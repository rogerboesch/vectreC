        SECTION code

_memcpy EXPORT


; void *memcpy(void *dest, const void *src, size_t n);
; Preserves U. Trashes D and X.
;
_memcpy pshs    u
        ldx     4,s             dest
        ldu     6,s             src
        tfr     u,d
        addd    8,s             add n to pointer to end of src string
        pshs    b,a
        bra     @cond
@loop
        lda     ,u+
        sta     ,x+
@cond
        cmpu    ,s              compare with end address
        bne     @loop

        leas    2,s             dispose of end address
        ldd     4,s             return dest
        puls    u,pc


        ENDSECTION
