	INCLUDE float.inc

	SECTION code

cmpSingleSignedInt	EXPORT


* Input: Stacked arguments: address of left packed single, right signed int.
* Output: B = -1, 0 or +1. CC reflects signed comparison of B with 0.
* Preserves X.
*
cmpSingleSignedInt
	pshs	u,y,x

	ldd	10,s		; right operand (signed int)
	flt_loadSignedDIntoFPA0

	ldx	8,s		; point to left operand (single)
	flt_compareFPA0ToX

	negb			; invert result because comparison was inverted
	cmpb	#0		; signed comparison, so no TSTB
	puls	x,y,u,pc




	ENDSECTION
