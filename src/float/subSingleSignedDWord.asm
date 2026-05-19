        INCLUDE float.inc

	SECTION code

subSingleSignedDWord	EXPORT

loadSignedDWordInFPA0   IMPORT
subSingle_common_add    IMPORT


subSingleSignedDWord
	pshs	u,y,x

	ldx	10,s		; load right operand
	lbsr	loadSignedDWordInFPA0
	com	FP0SGN		; negate FPA0

	; The left operand is loaded second in case the
	; preceding call trashes FPA1.

	ldx	8,s		; left (single)
	jsr	$BB2F		; unpack from X to FPA1

	lbra	subSingle_common_add




	ENDSECTION
