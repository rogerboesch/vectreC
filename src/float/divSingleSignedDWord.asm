	INCLUDE float.inc

	SECTION code

divSingleSignedDWord	EXPORT

isDWordZeroSpecial      IMPORT
divByZeroSingle_singleDividend IMPORT
loadSignedDWordInFPA0   IMPORT
unpackXToFPA1AndDiv     IMPORT


divSingleSignedDWord
	lbsr	isDWordZeroSpecial	; check right operand (divisor)
	lbeq	divByZeroSingle_singleDividend
	pshs	u,y,x
	ldx	10,s		; right (signed dword)
	lbsr	loadSignedDWordInFPA0
	ldx	8,s		; left (single)
	lbsr	unpackXToFPA1AndDiv
	ldx	,s		; result address
	flt_packFPA0ToX
	puls	x,y,u,pc




	ENDSECTION
