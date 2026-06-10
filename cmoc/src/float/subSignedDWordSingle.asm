	SECTION code

subSignedDWordSingle	EXPORT

loadSignedDWordInFPA0   IMPORT
subSingle_common        IMPORT


subSignedDWordSingle
	pshs	u,y,x
	ldx	8,s		; load left operand
	lbsr	loadSignedDWordInFPA0
	lbra	subSingle_common




	ENDSECTION
