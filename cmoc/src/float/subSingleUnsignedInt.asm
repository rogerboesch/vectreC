        INCLUDE float.inc

	SECTION code

subSingleUnsignedInt	EXPORT

loadUnsignedDInFPA0     IMPORT
subSingle_common_add    IMPORT


subSingleUnsignedInt
	pshs	u,y,x

	ldd	10,s		; load right operand
	lbsr	loadUnsignedDInFPA0
	flt_invertFPA0Sign

	; The left operand is loaded second in case the
	; preceding call trashes FPA1.

	ldx	8,s		; left (single)
	flt_unpackFromXToFPA1

	lbra	subSingle_common_add


	ENDSECTION
