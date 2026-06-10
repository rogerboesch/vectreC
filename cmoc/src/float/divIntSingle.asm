	INCLUDE float.inc

	SECTION code

divIntSingle	EXPORT

divByZeroSingle         IMPORT
unpackXToFPA0AndDiv     IMPORT

; Divided a signed 16-bit integer by a single-precision divisor.
; Input: X => Result single.
;        Pushed args: 1st = signed 16-bit signed integer dividend;
;                     2nd = address of single divisor.
; Output: Single stored at X.
;
divIntSingle
	tst	[4,s]		; check exponent of right operand (divisor)
        bne     @notDivBy0
        lda     2,s             ; MSB of left operand (signed int)
	lbra	divByZeroSingle ; fill result with +/- HUGE_VAL according to sign of A.
@notDivBy0
	pshs	u,y,x		; preserve result address (X); 6 bytes pushed
	ldd	8,s		; left (signed int, initially at 2,s)
	lbsr	loadSignedDInFPA1
	ldx	10,s		; right (single divisor, initially at 4,s)
	lbsr	unpackXToFPA0AndDiv
	ldx	,s		; result address
	flt_packFPA0ToX
	puls	x,y,u,pc

* Trashes FPA0.
loadSignedDInFPA1
	flt_loadSignedDIntoFPA0
	flt_copyFPA0ToFPA1
	rts




	ENDSECTION
