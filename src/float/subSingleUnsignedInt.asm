        INCLUDE float.inc

	SECTION code

subSingleUnsignedInt	EXPORT

loadUnsignedDInFPA0     IMPORT
subSingle_common_add    IMPORT


subSingleUnsignedInt
	pshs	u,y,x

	ldd	10,s		; load right operand
	lbsr	loadUnsignedDInFPA0
	com	FP0SGN		; negate FPA0

	; The left operand is loaded second in case the
	; preceding call trashes FPA1.

	ldx	8,s		; left (single)
	jsr	$BB2F		; unpack from X to FPA1

	lbra	subSingle_common_add




	ENDSECTION
