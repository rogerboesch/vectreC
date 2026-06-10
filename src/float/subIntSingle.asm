	INCLUDE float.inc

	SECTION code

subIntSingle	EXPORT

subSingle_common        IMPORT

subIntSingle
	pshs	u,y,x
	ldd	8,s		; load left operand
	flt_loadSignedDIntoFPA0
	lbra	subSingle_common




	ENDSECTION
