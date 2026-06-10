	INCLUDE float.inc

	SECTION code

mulUnsignedIntSingle	EXPORT

loadUnsignedDInFPA0     IMPORT
unpackXToFPA1AndMul     IMPORT


mulUnsignedIntSingle
	pshs	u,y,x
	ldd	8,s		; left (unsigned int)
	lbsr	loadUnsignedDInFPA0
	ldx	10,s		; right (single)
	lbsr	unpackXToFPA1AndMul
	ldx	,s		; result address
	flt_packFPA0ToX
	puls	x,y,u,pc




	ENDSECTION
