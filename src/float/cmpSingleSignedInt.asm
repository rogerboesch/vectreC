	SECTION code

cmpSingleSignedInt	EXPORT


* Input: Stacked arguments: address of left packed single, right signed int.
* Output: B = -1, 0 or +1. CC reflects signed comparison of B with 0.
* Preserves X.
*
cmpSingleSignedInt
	pshs	u,y,x

	ldd	10,s		; right operand (signed int)
	jsr	$B4F4		; load D (signed) into FPA0

	ldx	8,s		; point to left operand (single)
	jsr	$BC96		; compare FPA0 to X: puts -1, 0 or +1 in B, sets CC

	negb			; invert result because comparison was inverted
	cmpb	#0		; signed comparison, so no TSTB
	puls	x,y,u,pc




	ENDSECTION
