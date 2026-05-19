	SECTION code

divByZeroSingle	EXPORT


; Input: X => Result of division (packed single).
;
divByZeroSingle
	clr	,x		; store 0.0f in result
	rts



	ENDSECTION
