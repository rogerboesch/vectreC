	INCLUDE float.inc

	SECTION code

divSingleUnsignedDWord	EXPORT

isDWordZeroSpecial      IMPORT
divByZeroSingle_singleDividend IMPORT
loadUnsignedDWordInFPA0 IMPORT
unpackXToFPA1AndDiv     IMPORT


divSingleUnsignedDWord
	lbsr	isDWordZeroSpecial	; check right operand (divisor)
        lbeq    divByZeroSingle_singleDividend
	pshs	u,y,x
	ldx	10,s		; right (unsigned dword)
	lbsr	loadUnsignedDWordInFPA0
	ldx	8,s		; left (single)
	lbsr	unpackXToFPA1AndDiv
	ldx	,s		; result address
	flt_packFPA0ToX
	puls	x,y,u,pc




	ENDSECTION
