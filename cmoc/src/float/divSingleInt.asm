	INCLUDE float.inc

	SECTION code

divSingleInt	EXPORT

divByZeroSingle_singleDividend IMPORT
unpackXToFPA1AndDiv     IMPORT


; Preserves X.
;
divSingleInt
	ldd	4,s		; check right operand (divisor)
        lbeq    divByZeroSingle_singleDividend
	pshs	u,y,x
	ldd	10,s		; right (unsigned int)
	flt_loadSignedDIntoFPA0
	ldx	8,s		; left (single)
	lbsr	unpackXToFPA1AndDiv
	ldx	,s		; result address
	flt_packFPA0ToX
	puls	x,y,u,pc




	ENDSECTION
