	SECTION code

_strupr	EXPORT


* char *strupr(char *str)
_strupr
	ldx	2,s		str
	bra	@loopCond
@loop
	cmpa	#97		'a'
	blo	@loopCond
	cmpa	#122		'z'
	bhi	@loopCond
	suba	#32		make uppercase
	sta	-1,x
@loopCond
	lda	,x+
	bne	@loop
	ldd	2,s		return str
	rts


	ENDSECTION
