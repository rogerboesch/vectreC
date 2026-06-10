	INCLUDE float.inc

	SECTION code

addSignedDWordSingle	EXPORT

loadSignedDWordInFPA0   IMPORT
addFPA0FPA1             IMPORT

; Input: X => float that receives the sum.
; Pushed arguments:
;   signed long *
;   float *
;
addSignedDWordSingle
	pshs	u,y,x           ; 0..1,S is an argument passed to addFPA0FPA1
	ldx	8,s		; left (signed dword)
	lbsr	loadSignedDWordInFPA0
	ldx	10,s		; right (single)
	flt_unpackFromXToFPA1   ; preserves X
	lbra	addFPA0FPA1     ; pack sum at address stored in 0..1,S, i.e., original X

	ENDSECTION
