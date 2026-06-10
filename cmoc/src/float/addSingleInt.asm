	INCLUDE float.inc

	SECTION code

addSingleInt	EXPORT

addFPA0FPA1     IMPORT


addSingleInt
	pshs	u,y,x
	ldd	10,s		; right (signed int)
	flt_loadSignedDIntoFPA0
	ldx	8,s		; left (single)
	flt_unpackFromXToFPA1
	lbra	addFPA0FPA1




	ENDSECTION
