	INCLUDE float.inc

	SECTION code

mulSingleInt	EXPORT

unpackXToFPA1AndMul     IMPORT


mulSingleInt
	pshs	u,y,x
	ldd	10,s		; right (unsigned int)
	flt_loadSignedDIntoFPA0
	ldx	8,s		; left (single)
	lbsr	unpackXToFPA1AndMul
	ldx	,s		; result address
	flt_packFPA0ToX
	puls	x,y,u,pc




	ENDSECTION
