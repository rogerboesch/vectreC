	INCLUDE float.inc

	SECTION code

mulIntSingle	EXPORT

unpackXToFPA1AndMul     IMPORT


mulIntSingle
	pshs	u,y,x
	ldd	8,s		; right (unsigned int)
	flt_loadSignedDIntoFPA0
	ldx	10,s		; left (single)
	lbsr	unpackXToFPA1AndMul
	ldx	,s		; result address
	flt_packFPA0ToX
	puls	x,y,u,pc




	ENDSECTION
