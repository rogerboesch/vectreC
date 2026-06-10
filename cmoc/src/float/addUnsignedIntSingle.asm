	INCLUDE float.inc

	SECTION code

addUnsignedIntSingle	EXPORT

loadUnsignedDInFPA0     IMPORT
addFPA0FPA1             IMPORT


addUnsignedIntSingle
	pshs	u,y,x
	ldd	8,s		; left (unsigned int)
	lbsr	loadUnsignedDInFPA0
	ldx	10,s		; right (single)
	flt_unpackFromXToFPA1
	lbra	addFPA0FPA1




	ENDSECTION
