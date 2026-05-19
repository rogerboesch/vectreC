	SECTION code

divSingleUnsignedInt	EXPORT

divByZeroSingle         IMPORT
loadUnsignedDInFPA0     IMPORT
unpackXToFPA1AndDiv     IMPORT


divSingleUnsignedInt
	ldd	4,s		; check right operand (divisor)
	lbeq	divByZeroSingle
	pshs	u,y,x
	ldd	10,s		; right (unsigned int)
	lbsr	loadUnsignedDInFPA0
	ldx	8,s		; left (single)
	lbsr	unpackXToFPA1AndDiv
	ldx	,s		; result address
	jsr	$BC35		; pack FPA0 into X
	puls	x,y,u,pc




	ENDSECTION
