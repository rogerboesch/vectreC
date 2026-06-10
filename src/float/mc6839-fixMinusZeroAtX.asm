        INCLUDE float.inc


fixMinusZeroAtX EXPORT


        SECTION code


; Input: X => Single-precision float.
; Remove minus sign on -0.0.
; This has been seen when multiplying -5.678f by 0.0f,
; and when dividing 0.0f by -432.888f.
; Preserves X. Trashes D.
;
fixMinusZeroAtX
        ldd     ,x
        addd    #$8000
        bne     @done
        ldd     2,x
        bne     @done
        stb     ,x              ; clear sign bit
@done
        rts


        ENDSECTION
