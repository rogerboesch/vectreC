	INCLUDE float.inc

	SECTION code

subSingle_common	EXPORT
subSingle_common_add    EXPORT


; Routine that finishes work started by other routines, e.g., subSignedDWordSingle.
; Input: FPA0 = left operand;
;        10,S (before call) => right operand (single-precision).
; Output: ,S (before call) => address where to pack the result.
; Preserves X, Y, U. Trashes D.
;
subSingle_common
	ldx	10,s		; right (single)
	flt_unpackFromXToFPA1
	flt_invertFPA1Sign

        flt_computeResultSign

;   FALLTHROUGH

; Routine that finishes work started by other routines, e.g., subSingleSignedDWord.
;
subSingle_common_add
	flt_addFPA1ToFPA0

	ldx	,s		; result
	flt_packFPA0ToX
	puls	x,y,u,pc


	ENDSECTION
