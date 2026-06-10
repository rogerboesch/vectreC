	INCLUDE float.inc

	SECTION code

loadSignedDWordInFPA0	EXPORT


; Input: X => unsigned dword.
;
loadSignedDWordInFPA0

        IFDEF _CMOC_MC6839_

initSingleFromDWord IMPORT
        tfr     x,d             ; point D to dword
        leax    FP0ADDR,pcr     ; destination
        orcc    #1              ; dword is signed
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
	flt_loadBFromTopFPA0MantissaByte
	subb	#$80		; set carry if value is non-negative
	flt_normalizeFPA0
	rts

        ENDC


	ENDSECTION
