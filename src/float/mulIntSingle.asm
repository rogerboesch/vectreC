	SECTION code

mulIntSingle	EXPORT

unpackXToFPA1AndMul     IMPORT


mulIntSingle
	pshs	u,y,x
	ldd	8,s		; right (unsigned int)
	jsr	$B4F4		; load D (signed) into FPA0
	ldx	10,s		; left (single)
	lbsr	unpackXToFPA1AndMul
	ldx	,s		; result address
	jsr	$BC35		; pack FPA0 into X
	puls	x,y,u,pc




	ENDSECTION
