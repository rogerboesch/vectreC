	SECTION code

_strlwr	EXPORT


* char *strlwr(char *str)
_strlwr
	ldx	2,s		str
	bra	@loopCond
@loop
	cmpa	#65		'A'
	blo	@loopCond
	cmpa	#90		'Z'
	bhi	@loopCond
	adda	#32		make lowercase
	sta	-1,x
@loopCond
	lda	,x+
	bne	@loop
	ldd	2,s		return str
	rts


	ENDSECTION
