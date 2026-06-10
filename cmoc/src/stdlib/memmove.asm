        SECTION code

_memmove        EXPORT

_memcpy         IMPORT


; void *memmove(void *dest, const void *src, size_t n)
; 
; Preserves U. Trashes D, X.
;
_memmove
        pshs    u
; 4,s = dest; 6,s = src; 8,s = n.
        ldd     8,s     ; n
        beq     @done
; If the source is at a higher address than the destination, use memcpy().
; If the end of the destination region is at or below the source, use memcpy().
        ldx     6,s     ; src
        cmpx    4,s     ; dest
        bhs     @memcpy
        ldd     4,s
        addd    8,s     ; dest + n
        pshs    b,a
        cmpx    ,s      ; compare src in X with dest + n in stack
        puls    u       ; pop dest + n (does not affect CC)
        bhs     @memcpy
; Copy backwards.
; Here, X = src, U = dest + n.
        ldd     8,s     ; n
        leax    d,x     ; src + n
; Copy a byte until X has gone back to src.
@byteCopyLoop
        lda     ,-x
        sta     ,-u
        cmpx    6,s     ; compare with src
        bhi     @byteCopyLoop
        bra     @done
;
@memcpy
        ldu     8,s         ; n (3rd argument of memcpy())
        ldd     4,s         ; dest (1st argument)
        ; X already contains scr
        pshs    u,x,b,a     ; push memcpy() args in reverse: U (n), X (src), D (dest)
        lbsr    _memcpy
        leas    6,s
@done
        ldd     4,s         ; return dest
        puls    u,pc


        ENDSECTION
