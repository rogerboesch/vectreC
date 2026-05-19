	SECTION code

addIntSingle	EXPORT

addFPA0FPA1     IMPORT

addIntSingle
	pshs	u,y,x
	ldd	8,s		; right (signed int)
	jsr	$B4F4		; load D (signed) into FPA0
	ldx	10,s		; left (single)
	jsr	$BB2F		; unpack from X to FPA1
	lbra	addFPA0FPA1




	ENDSECTION
