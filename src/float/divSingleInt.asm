	SECTION code

divSingleInt	EXPORT

divByZeroSingle         IMPORT
unpackXToFPA1AndDiv     IMPORT


divSingleInt
	ldd	4,s		; check right operand (divisor)
	lbeq	divByZeroSingle
	pshs	u,y,x
	ldd	10,s		; right (unsigned int)
	jsr	$B4F4		; load D (signed) into FPA0
	ldx	8,s		; left (single)
	lbsr	unpackXToFPA1AndDiv
	ldx	,s		; result address
	jsr	$BC35		; pack FPA0 into X
	puls	x,y,u,pc




	ENDSECTION
