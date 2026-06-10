	INCLUDE float.inc

	SECTION code

loadUnsignedDWordInFPA0	EXPORT


; Input: X => unsigned dword.
;
loadUnsignedDWordInFPA0

        IFDEF _CMOC_MC6839_

initSingleFromDWord IMPORT
        tfr     x,d
        leax    FP0ADDR,pcr
        andcc   #$FE                    ; dword is unsigned
        lbra    initSingleFromDWord

        ELSE

	flt_clrVALTYP
	ldd	,x
	std	FP0MAN		; store in upper mantissa of FPA0
	ldd	2,x
	std	FP0MAN+2	; store in upper mantissa of FPA0
	ldb	#EXPBIAS+32 	; 32 = exponent required
	flt_storeBInFPA0BiasedExp
	flt_clrFPSBYT
	flt_clearFPA0Sign
	orcc	#1		; set carry so value seen as non-negative
	flt_normalizeFPA0
	rts

        ENDC


	ENDSECTION
