        SECTION code

pushSmallStruct EXPORT


; Copies a region of memory into the stack.
; X: Address of the region to read.
; B: Size in bytes of the region. Must not be zero.
; S: Address where to copy the region to.
; Example:
;       LEAX    destination,PCR
;       LDB     #17
;       LEAS    -17,S
;       LBSR    pushSmallStruct
; Preserves U, Y. Trashes A, B, X. Returns nothing.
;
pushSmallStruct
        pshs    u
        leau    4,s             destination (past pushed U and return address)
@copyByte
        lda     ,x+
        sta     ,u+
        decb
        bne     @copyByte
        puls    u,pc


        ENDSECTION
