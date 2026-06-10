	INCLUDE float.inc

	SECTION code

addSingleUnsignedDWord	EXPORT

loadUnsignedDWordInFPA0 IMPORT
addFPA0FPA1             IMPORT


addSingleUnsignedDWord
	pshs	u,y,x
	ldx	10,s		; right (unsigned dword)
	lbsr	loadUnsignedDWordInFPA0
	ldx	8,s		; left (single)
	flt_unpackFromXToFPA1
	lbra	addFPA0FPA1




	ENDSECTION
