        INCLUDE float.inc

	SECTION code

addFPA0FPA1	EXPORT


* Input: FPA1 = number to add to FPA0.
*        Pushed argument = address of resulting packed float.
* Output: Sum (packed single) stored at X.
* Trashes FPA0.
* Preserves X.
*
addFPA0FPA1
	lda     FP1EXP		; load exponent of FPA1
	ldb     FP0EXP		; load exponent of FPA0
	jsr     $B9C5		; FPA0 += FPA1

	ldx	,s		; result
	jsr	$BC35		; pack FPA0 into X
	puls	x,y,u,pc




	ENDSECTION
