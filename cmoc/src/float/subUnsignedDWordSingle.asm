	SECTION code

subUnsignedDWordSingle	EXPORT

loadUnsignedDWordInFPA0 IMPORT
subSingle_common        IMPORT


subUnsignedDWordSingle
	pshs	u,y,x
	ldx	8,s		; load left operand
	lbsr	loadUnsignedDWordInFPA0
	lbra	subSingle_common




	ENDSECTION
