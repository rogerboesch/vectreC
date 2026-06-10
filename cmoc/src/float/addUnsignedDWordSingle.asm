	INCLUDE float.inc

	SECTION code

addUnsignedDWordSingle	EXPORT

loadUnsignedDWordInFPA0 IMPORT
addFPA0FPA1             IMPORT


addUnsignedDWordSingle
	pshs	u,y,x
	ldx	8,s		; left (unsigned dword)
	lbsr	loadUnsignedDWordInFPA0
	ldx	10,s		; right (single)
	flt_unpackFromXToFPA1
	lbra	addFPA0FPA1




	ENDSECTION
