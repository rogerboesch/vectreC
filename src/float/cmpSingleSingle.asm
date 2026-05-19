	SECTION code

cmpSingleSingle	EXPORT


* Input: Stacked arguments: address of left packed single, address of right.
* Output: B = -1, 0 or +1. CC reflects signed comparison of B with 0.
* Preserves X.
*
cmpSingleSingle
	pshs	u,y,x

	ldx	8,s		; point to left operand (single)
	jsr	$BC14		; unpack from X to FPA0

	ldx	10,s		; point to right operand (single)
	jsr	$BC96		; compare FPA0 to X: puts -1, 0 or +1 in B, sets CC

	puls	x,y,u,pc




	ENDSECTION
