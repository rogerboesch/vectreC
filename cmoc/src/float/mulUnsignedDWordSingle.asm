	INCLUDE float.inc

	SECTION code

mulUnsignedDWordSingle	EXPORT

loadUnsignedDWordInFPA0 IMPORT
unpackXToFPA1AndMul     IMPORT


mulUnsignedDWordSingle
	pshs	u,y,x
	ldx	8,s		; left (unsigned dword)
	lbsr	loadUnsignedDWordInFPA0
	ldx	10,s		; right (single)
	lbsr	unpackXToFPA1AndMul
	ldx	,s		; result address
	flt_packFPA0ToX
	puls	x,y,u,pc




	ENDSECTION
