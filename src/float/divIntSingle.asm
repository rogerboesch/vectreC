        INCLUDE float.inc

	SECTION code

divIntSingle	EXPORT

divByZeroSingle         IMPORT
unpackXToFPA0AndDiv     IMPORT


divIntSingle
	tst	[4,s]		; check exponent of right operand (divisor)
	lbeq	divByZeroSingle
	pshs	u,y,x
	ldd	8,s		; left (signed int)
	lbsr	loadSignedDInFPA1
	ldx	10,s		; left (single)
	lbsr	unpackXToFPA0AndDiv
	ldx	,s		; result address
	jsr	$BC35		; pack FPA0 into X
	puls	x,y,u,pc

* Trashes FPA0.
loadSignedDInFPA1
	jsr	$B4F4		; load D (signed) into FPA0
	ldd	FP0ADDR
	std	FP1ADDR
	ldd	FP0ADDR+2
	std	FP1ADDR+2
	ldd	FP0ADDR+4
	std	FP1ADDR+4
	rts




	ENDSECTION
