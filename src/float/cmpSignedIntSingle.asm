	SECTION code

cmpSignedIntSingle	EXPORT


cmpSignedIntSingle
	pshs	u,y,x

	ldd	8,s		; left operand (signed int)
	jsr	$B4F4		; load D (signed) into FPA0

	ldx	10,s		; point to right operand (single)
	jsr	$BC96		; compare FPA0 to X: puts -1, 0 or +1 in B, sets CC

	puls	x,y,u,pc




	ENDSECTION
