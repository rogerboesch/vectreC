	INCLUDE float.inc

	SECTION code

loadUnsignedDInFPA0	EXPORT


; Trashes D, X.
;
loadUnsignedDInFPA0

        IFDEF _CMOC_MC6839_

initSingleFromUnsignedWord IMPORT
        leax    FP0ADDR,pcr
        lbra    initSingleFromUnsignedWord

        ELSE

	flt_clrVALTYP
	std	FP0MAN		; store in upper mantissa of FPA0
	ldb	#EXPBIAS+16 	; 16 = exponent required
	flt_storeBInFPA0BiasedExp
	clra
	clrb
	std	FP0MAN+2    ; clear lower mantissa of FPA0
	flt_clrFPSBYT
	flt_clearFPA0Sign
	orcc	#1		; set carry so value seen as non-negative
	flt_normalizeFPA0
	rts

        ENDC


	ENDSECTION
