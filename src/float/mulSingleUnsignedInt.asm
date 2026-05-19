	SECTION code

mulSingleUnsignedInt	EXPORT

loadUnsignedDInFPA0     IMPORT
unpackXToFPA1AndMul     IMPORT


mulSingleUnsignedInt
	pshs	u,y,x
	ldd	10,s		; right (unsigned int)
	lbsr	loadUnsignedDInFPA0
	ldx	8,s		; left (single)
	lbsr	unpackXToFPA1AndMul
	ldx	,s		; result address
	jsr	$BC35		; pack FPA0 into X
	puls	x,y,u,pc




	ENDSECTION
