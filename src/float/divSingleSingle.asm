	SECTION code

divSingleSingle	EXPORT

divByZeroSingle         IMPORT
binOpSingleSingle       IMPORT


; Divides two numbers and writes the result at a third location.
; Synopsis:
;	pshs	rightOpAddr
;	pshs	leftOpAddr
;	leax	result,PCR
;	lbsr	divSingleSingle
;	leas	4,s
; Preserves X.
;
divSingleSingle
	tst	[4,s]		; check exponent of right operand (divisor)
	lbeq	divByZeroSingle
	pshs	u,y,x
	ldu	#$BB8F		; unpack from X to FPA1; FP0 = FPA1 / FPA0
	lbsr	binOpSingleSingle
	puls	x,y,u,pc




	ENDSECTION
