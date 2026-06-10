	SECTION code

_strlen	EXPORT


_strlen:
	ldx	2,s
@loop
	tst	,x+
	bne	@loop

	tfr	x,d
	subd	#1		compensate for increment past '\0'
	subd	2,s		substract start address

	rts


	ENDSECTION
