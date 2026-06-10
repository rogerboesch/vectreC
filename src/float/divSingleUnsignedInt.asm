	INCLUDE float.inc

	SECTION code

divSingleUnsignedInt	EXPORT

divByZeroSingle_singleDividend IMPORT
loadUnsignedDInFPA0     IMPORT
unpackXToFPA1AndDiv     IMPORT


divSingleUnsignedInt
	ldd	4,s		; check right operand (divisor)
        lbeq    divByZeroSingle_singleDividend
	pshs	u,y,x
	ldd	10,s		; right (unsigned int)
	lbsr	loadUnsignedDInFPA0
	ldx	8,s		; left (single)
	lbsr	unpackXToFPA1AndDiv
	ldx	,s		; result address
	flt_packFPA0ToX
	puls	x,y,u,pc




	ENDSECTION
