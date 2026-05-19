	SECTION code

mulSignedDWordSingle	EXPORT

loadSignedDWordInFPA0   IMPORT
unpackXToFPA1AndMul     IMPORT


mulSignedDWordSingle
	pshs	u,y,x
	ldx	8,s		; left (unsigned dword)
	lbsr	loadSignedDWordInFPA0
	ldx	10,s		; right (single)
	lbsr	unpackXToFPA1AndMul
	ldx	,s		; result address
	jsr	$BC35		; pack FPA0 into X
	puls	x,y,u,pc




	ENDSECTION
