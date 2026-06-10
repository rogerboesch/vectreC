	INCLUDE float.inc

	SECTION code

subSingleInt	EXPORT

subSingle_common_add	IMPORT

subSingleInt
	pshs	u,y,x

	clra
	clrb
	subd	10,s		; load right operand, negated
	flt_loadSignedDIntoFPA0

	; The left operand must be loaded second because GIVABF
	; appears to trash FPA1.

	ldx	8,s		; left (single)
	flt_unpackFromXToFPA1

	lbra	subSingle_common_add




	ENDSECTION
