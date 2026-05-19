	SECTION code

mulSingleUnsignedDWord	EXPORT

loadUnsignedDWordInFPA0 IMPORT
unpackXToFPA1AndMul     IMPORT


mulSingleUnsignedDWord
	pshs	u,y,x
	ldx	10,s		; right (unsigned dword)
	lbsr	loadUnsignedDWordInFPA0
	ldx	8,s		; left (single)
	lbsr	unpackXToFPA1AndMul
	ldx	,s		; result address
	jsr	$BC35		; pack FPA0 into X
	puls	x,y,u,pc




	ENDSECTION
