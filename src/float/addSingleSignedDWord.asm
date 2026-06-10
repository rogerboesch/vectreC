	INCLUDE float.inc

	SECTION code

addSingleSignedDWord	EXPORT

loadSignedDWordInFPA0   IMPORT
addFPA0FPA1             IMPORT


addSingleSignedDWord
	pshs	u,y,x
	ldx	10,s		; right (unsigned dword)
	lbsr	loadSignedDWordInFPA0
	ldx	8,s		; left (single)
	flt_unpackFromXToFPA1
	lbra	addFPA0FPA1




	ENDSECTION
