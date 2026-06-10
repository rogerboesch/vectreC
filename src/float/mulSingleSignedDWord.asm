	INCLUDE float.inc

	SECTION code

mulSingleSignedDWord	EXPORT

loadSignedDWordInFPA0   IMPORT
unpackXToFPA1AndMul     IMPORT


mulSingleSignedDWord
	pshs	u,y,x
	ldx	10,s		; right (unsigned dword)
	lbsr	loadSignedDWordInFPA0
	ldx	8,s		; left (single)
	lbsr	unpackXToFPA1AndMul
	ldx	,s		; result address
	flt_packFPA0ToX
	puls	x,y,u,pc




	ENDSECTION
