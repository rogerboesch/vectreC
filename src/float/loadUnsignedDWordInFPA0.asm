        INCLUDE float.inc

	SECTION code

loadUnsignedDWordInFPA0	EXPORT


; Input: X => unsigned dword.
;
loadUnsignedDWordInFPA0
	clr	VALTYP		; set value type to numeric
	ldd	,x
	std	FP0MAN		; store in upper mantissa of FPA0
	ldd	2,x
	std	FP0MAN+2	; store in upper mantissa of FPA0
	ldb	#128+32 	; 32 = exponent required
	stb	FP0EXP
	clr	FPSBYT
	clr	FP0SGN
	orcc	#1		; set carry so value seen as non-negative (see $BA18)
	jmp	$BA18		; normalize FPA0 (reads carry)



	ENDSECTION
