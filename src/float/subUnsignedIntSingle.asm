	SECTION code

subUnsignedIntSingle	EXPORT

loadUnsignedDInFPA0     IMPORT
subSingle_common        IMPORT


subUnsignedIntSingle
	pshs	u,y,x
	ldd	8,s		; load left operand
	lbsr	loadUnsignedDInFPA0
	lbra	subSingle_common




	ENDSECTION
