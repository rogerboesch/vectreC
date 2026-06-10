	INCLUDE float.inc

	SECTION code

cmpSingleSingle	EXPORT


* Input: Stacked arguments: address of left packed single, address of right.
* Output: B = -1, 0 or +1. CC reflects signed comparison of B with 0.
* Preserves X.
*
cmpSingleSingle
	pshs	u,y,x

	ldx	8,s		; point to left operand (single)
	flt_unpackFromXToFPA0

	ldx	10,s		; point to right operand (single)
	flt_compareFPA0ToX

	puls	x,y,u,pc




	ENDSECTION
