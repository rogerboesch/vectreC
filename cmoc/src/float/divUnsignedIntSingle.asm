	INCLUDE float.inc

	SECTION code

divUnsignedIntSingle	EXPORT

divByZeroSingle         IMPORT
loadUnsignedDInFPA0     IMPORT
unpackXToFPA0AndDiv     IMPORT


divUnsignedIntSingle
	tst	[4,s]		; check exponent of right operand (divisor)
        bne     @notDivBy0
        clra                    ; dividend never negative
	lbra	divByZeroSingle
@notDivBy0
	pshs	u,y,x
	ldd	8,s		; left (unsigned int)
	lbsr	loadUnsignedDInFPA1
	ldx	10,s		; right (single)
	lbsr	unpackXToFPA0AndDiv
	ldx	,s		; result address
	flt_packFPA0ToX
	puls	x,y,u,pc

* Trashes FPA0.
loadUnsignedDInFPA1
	lbsr	loadUnsignedDInFPA0
	flt_copyFPA0ToFPA1
	rts




	ENDSECTION
