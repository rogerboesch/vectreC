	SECTION code

subSingleInt	EXPORT

subSingle_common_add	IMPORT

subSingleInt
	pshs	u,y,x

	clra
	clrb
	subd	10,s		; load right operand, negated
	jsr	$B4F4		; load D (signed) into FPA0

	; The left operand must be loaded second because $B4F4
	; appears to trash FPA1.

	ldx	8,s		; left (single)
	jsr	$BB2F		; unpack from X to FPA1

	lbra	subSingle_common_add




	ENDSECTION
