	SECTION code

negateSingle	EXPORT


; Negates the packed single-precision float at X.
; Preserves X.
;
negateSingle
	ldb	1,x
	eorb	#$80
	stb	1,x
	rts




	ENDSECTION
