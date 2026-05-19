	SECTION code

divSingleSignedDWord	EXPORT

isDWordZeroSpecial      IMPORT
divByZeroSingle         IMPORT
loadSignedDWordInFPA0   IMPORT
unpackXToFPA1AndDiv     IMPORT


divSingleSignedDWord
	lbsr	isDWordZeroSpecial	; check right operand (divisor)
	lbeq	divByZeroSingle
@noDivBy0
	pshs	u,y,x
	ldx	10,s		; right (signed dword)
	lbsr	loadSignedDWordInFPA0
	ldx	8,s		; left (single)
	lbsr	unpackXToFPA1AndDiv
	ldx	,s		; result address
	jsr	$BC35		; pack FPA0 into X
	puls	x,y,u,pc




	ENDSECTION
