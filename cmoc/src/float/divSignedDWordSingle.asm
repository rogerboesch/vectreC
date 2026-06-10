	INCLUDE float.inc

	SECTION code

divSignedDWordSingle	EXPORT

divByZeroSingle         IMPORT
unpackXToFPA0AndDiv     IMPORT
loadSignedDWordInFPA0   IMPORT


; Pushed args: 1st = left (address of signed dword), 2nd = ; right (address of single).
; Preserves X.
;
divSignedDWordSingle
	tst	[4,s]		; check exponent of right operand (divisor)
        bne     @notDivBy0
        lda     [2,s]           ; load MSB of signed dword (left operand)
	lbra	divByZeroSingle
@notDivBy0
	pshs	u,y,x
	ldx	8,s		; left (signed dword)
	lbsr	loadSignedDWordInFPA1
	ldx	10,s		; right (single)
	lbsr	unpackXToFPA0AndDiv
	ldx	,s		; result address
	flt_packFPA0ToX
	puls	x,y,u,pc

* Trashes FPA0.
loadSignedDWordInFPA1
	lbsr	loadSignedDWordInFPA0
	flt_copyFPA0ToFPA1
	rts




	ENDSECTION
