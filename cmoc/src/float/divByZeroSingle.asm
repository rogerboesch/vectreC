	SECTION code

divByZeroSingle	EXPORT


; Sets the packed single at X with the largest value of the sign specified by bit 7 of A.
; Input: Bit 7 of A: 1 to get negative value, 0 for positive.
;        X => Result of division (packed single).
; Trashes A. Preserves B, X.
;
divByZeroSingle
        ora     #$7F            ; keep bit 7 (sign bit), set all mantissa bits
        IFDEF _CMOC_MC6839_
        sta     ,x
        lda     #$FF            ; rest of packed float is $FF
        sta     1,x
        sta     2,x             ; fill rest of mantissa
        sta     3,x
        ELSE
        sta     1,x             ; store high byte (sign bit and 7 high bits of mantissa)
        lda     #$FF            ; rest of packed float is $FF
	sta	,x              ; store exponent (127 + bias)
        sta     2,x             ; fill rest of mantissa
        sta     3,x
        sta     4,x
        ENDC
	rts


	ENDSECTION
