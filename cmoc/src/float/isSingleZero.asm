	SECTION code

isSingleZero    EXPORT


; Sets the Z flag to 1 if the packed single-precision float at X is zero.
; Sets the Z flag to 0 otherwise.
; Preserves X and D.
;
isSingleZero
	tst	,x		; null exponent byte means number is zero
	rts


	ENDSECTION
