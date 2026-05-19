	SECTION code

addSingleInt	EXPORT

addFPA0FPA1     IMPORT


addSingleInt
	pshs	u,y,x
	ldd	10,s		; right (signed int)
	jsr	$B4F4		; load D (signed) into FPA0
	ldx	8,s		; left (single)
	jsr	$BB2F		; unpack from X to FPA1
	lbra	addFPA0FPA1




	ENDSECTION
