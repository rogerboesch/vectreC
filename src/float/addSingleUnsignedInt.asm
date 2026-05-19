	SECTION code

addSingleUnsignedInt	EXPORT

loadUnsignedDInFPA0     IMPORT
addFPA0FPA1             IMPORT


addSingleUnsignedInt
	pshs	u,y,x
	ldd	10,s		; right (unsigned int)
	lbsr	loadUnsignedDInFPA0
	ldx	8,s		; left (single)
	jsr	$BB2F		; unpack from X to FPA1
	lbra	addFPA0FPA1




	ENDSECTION
