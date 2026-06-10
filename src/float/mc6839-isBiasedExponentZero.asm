	INCLUDE float.inc


isBiasedExponentZero EXPORT


        SECTION code


; Input: D = Top word of a single-precision float. Bits 14-7 contain the biased exponent.
; Output: Z=1 iff the biased exponent is 0. D unchanged.
;
isBiasedExponentZero
        bita    #$7F
        bne     @notZero
        bitb    #$80
@notZero
        rts


        ENDSECTION
