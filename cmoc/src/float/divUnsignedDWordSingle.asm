	INCLUDE float.inc

	SECTION code

divUnsignedDWordSingle	EXPORT

divByZeroSingle         IMPORT
loadUnsignedDWordInFPA0 IMPORT
unpackXToFPA0AndDiv     IMPORT


divUnsignedDWordSingle
	tst	[4,s]		; check exponent of right operand (divisor)
        bne     @notDivBy0
        clra                    ; dividend never negative
        lbra    divByZeroSingle
@notDivBy0
	pshs	u,y,x
	ldx	8,s		; left (unsigned dword)
	lbsr	loadUnsignedDWordInFPA1
	ldx	10,s		; right (single)
	lbsr	unpackXToFPA0AndDiv
	ldx	,s		; result address
	flt_packFPA0ToX
	puls	x,y,u,pc

* Trashes FPA0.
loadUnsignedDWordInFPA1
	lbsr	loadUnsignedDWordInFPA0
	flt_copyFPA0ToFPA1
	rts




	ENDSECTION
