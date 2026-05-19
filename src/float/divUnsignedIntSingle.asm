        INCLUDE float.inc

	SECTION code

divUnsignedIntSingle	EXPORT

divByZeroSingle         IMPORT
loadUnsignedDInFPA0     IMPORT
unpackXToFPA0AndDiv     IMPORT


divUnsignedIntSingle
	tst	[4,s]		; check exponent of right operand (divisor)
	lbeq	divByZeroSingle
	pshs	u,y,x
	ldd	8,s		; left (unsigned int)
	lbsr	loadUnsignedDInFPA1
	ldx	10,s		; right (single)
	lbsr	unpackXToFPA0AndDiv
	ldx	,s		; result address
	jsr	$BC35		; pack FPA0 into X
	puls	x,y,u,pc

* Trashes FPA0.
loadUnsignedDInFPA1
	lbsr	loadUnsignedDInFPA0
	ldd	FP0ADDR
	std	FP1ADDR
	ldd	FP0ADDR+2
	std	FP1ADDR+2
	ldd	FP0ADDR+4
	std	FP1ADDR+4
	rts




	ENDSECTION
