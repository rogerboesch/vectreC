	SECTION code

divSingleUnsignedDWord	EXPORT

isDWordZeroSpecial      IMPORT
divByZeroSingle         IMPORT
loadUnsignedDWordInFPA0 IMPORT
unpackXToFPA1AndDiv     IMPORT


divSingleUnsignedDWord
	lbsr	isDWordZeroSpecial	; check right operand (divisor)
	lbeq	divByZeroSingle
@noDivBy0
	pshs	u,y,x
	ldx	10,s		; right (unsigned dword)
	lbsr	loadUnsignedDWordInFPA0
	ldx	8,s		; left (single)
	lbsr	unpackXToFPA1AndDiv
	ldx	,s		; result address
	jsr	$BC35		; pack FPA0 into X
	puls	x,y,u,pc




	ENDSECTION
