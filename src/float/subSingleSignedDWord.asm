        INCLUDE float.inc

	SECTION code

subSingleSignedDWord	EXPORT

loadSignedDWordInFPA0   IMPORT
subSingle_common_add    IMPORT


subSingleSignedDWord
	pshs	u,y,x

	ldx	10,s		; load right operand
	lbsr	loadSignedDWordInFPA0
	flt_invertFPA0Sign

	; The left operand is loaded second in case the
	; preceding call trashes FPA1.

	ldx	8,s		; left (single)
	flt_unpackFromXToFPA1

	lbra	subSingle_common_add


	ENDSECTION
