	INCLUDE float.inc

	SECTION code

addIntSingle	EXPORT

addFPA0FPA1     IMPORT

addIntSingle
	pshs	u,y,x
	ldd	8,s		; right (signed int)
	flt_loadSignedDIntoFPA0     ; defined in float.inc
	ldx	10,s		; left (single)
	flt_unpackFromXToFPA1
	lbra	addFPA0FPA1




	ENDSECTION
