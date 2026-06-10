	SECTION code


resetDBytes EXPORT


; Input: X => Where to write null bytes.
;        D => How many null bytes to write (must NOT be 0).
; Trashes D, X.
;
resetDBytes
        clr     ,x+
        subd    #1
        bne     resetDBytes
        rts


	ENDSECTION
