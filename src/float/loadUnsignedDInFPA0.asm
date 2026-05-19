        INCLUDE float.inc

	SECTION code

loadUnsignedDInFPA0	EXPORT


loadUnsignedDInFPA0
	clr	VALTYP		; set variable type (VALTYP) to numeric
	std	FP0MAN		; store in upper mantissa of FPA0
	ldb	#128+16 	; 16 = exponent required
	orcc	#1		; set carry so value seen as non-negative (see $BA18)
	jmp	$BC86		; continue with rest of routine started at $B4F4




	ENDSECTION
