	INCLUDE float.inc

	SECTION code

cmpSignedIntSingle	EXPORT


cmpSignedIntSingle
	pshs	u,y,x

	ldd	8,s		; left operand (signed int)
	flt_loadSignedDIntoFPA0

	ldx	10,s		; point to right operand (single)
	flt_compareFPA0ToX

	puls	x,y,u,pc




	ENDSECTION
