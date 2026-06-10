	SECTION code


resetBBytes EXPORT


; Input: X => Where to write null bytes.
;        B => How many null bytes to write (must NOT be 0).
; Trashes B, X.
;
resetBBytes
        clr     ,x+
        decb
        bne     resetBBytes
        rts


	ENDSECTION
