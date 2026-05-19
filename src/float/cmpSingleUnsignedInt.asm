	SECTION code

cmpSingleUnsignedInt	EXPORT

loadUnsignedDInFPA0     IMPORT


cmpSingleUnsignedInt
	pshs	u,y,x

	ldd	10,s		        ; right operand (unsigned int)
	lbsr	loadUnsignedDInFPA0	; load D (unsigned) into FPA0

	ldx	8,s		; point to left operand (single)
	jsr	$BC96		; compare FPA0 to X: puts -1, 0 or +1 in B, sets CC

	negb			; invert result because comparison was inverted
	cmpb	#0		; signed comparison, so no TSTB
	puls	x,y,u,pc




	ENDSECTION
