	INCLUDE float.inc

	SECTION code

cmpUnsignedIntSingle	EXPORT

loadUnsignedDInFPA0     IMPORT


cmpUnsignedIntSingle
	pshs	u,y,x

	ldd	8,s		        ; left operand (unsigned int)
	lbsr	loadUnsignedDInFPA0	; load D (unsigned) into FPA0

	ldx	10,s		; point to right operand (single)
	flt_compareFPA0ToX

	puls	x,y,u,pc



	ENDSECTION
