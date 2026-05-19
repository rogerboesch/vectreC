        INCLUDE float.inc

	SECTION code

loadSignedDWordInFPA0	EXPORT


; Input: X => unsigned dword.
;
loadSignedDWordInFPA0
	clr	VALTYP		; set value type to numeric
	ldd	,x
	std	FP0MAN		; store in upper mantissa of FPA0
	ldd	2,x
	std	FP0MAN+2	; store in upper mantissa of FPA0
	ldb	#128+32 	; 32 = exponent required
	stb	FP0EXP
	clr	FPSBYT
	clr	FP0SGN
	ldb	FP0MAN
	subb	#$80		; set carry if value is non-negative (see $BA18)
	jmp	$BA18		; normalize FPA0 (reads carry)




	ENDSECTION
