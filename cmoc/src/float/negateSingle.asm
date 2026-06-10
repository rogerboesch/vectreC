	SECTION code

negateSingle	EXPORT


; Negates the packed single-precision float at X.
; Preserves X. Trashes D.
;
negateSingle

        IFDEF _CMOC_MC6839_
;
isBiasedExponentZero IMPORT
        ldd     ,x              ; load word that contains the biased exponent
        lbsr    isBiasedExponentZero
        beq     @done
        eora    #$80
        sta     ,x
;
        ELSE
;
        tst     ,x              ; check exponent: is number zero?
        beq     @done           ; if yes, nothing to do
	ldb	1,x
	eorb	#$80
	stb	1,x
;
        ENDC
@done
	rts




	ENDSECTION
