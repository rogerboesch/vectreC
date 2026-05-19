	SECTION code

subIntSingle	EXPORT

subSingle_common        IMPORT

subIntSingle
	pshs	u,y,x
	ldd	8,s		; load left operand
	jsr	$B4F4		; load D (signed) into FPA0
	lbra	subSingle_common




	ENDSECTION
