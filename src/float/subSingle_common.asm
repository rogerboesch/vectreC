        INCLUDE float.inc

	SECTION code

subSingle_common	EXPORT
subSingle_common_add    EXPORT


; Input: FPA0 = left operand;
;        10,S (before call) => right operand (single-precision).
; Output: ,S (before call) => address where to pack the result.
; Preserves X, Y, U. Trashes D.
;
subSingle_common
	ldx	10,s		; right (single)
	jsr	$BB2F		; unpack from X to FPA1
	com	FP1SGN		; invert sign of FPA1

        ; Compute sign of result.
        ldb     FP0SGN
        eorb    FP1SGN
        stb     RESSGN

subSingle_common_add
	lda     FP1EXP		; load exponent of FPA1
	ldb     FP0EXP		; load exponent of FPA0
	jsr     $B9C5		; FPA0 += FPA1

	ldx	,s		; result
	jsr	$BC35		; pack FPA0 into X
	puls	x,y,u,pc




	ENDSECTION
