	SECTION code

subSingleSingle	EXPORT

binOpSingleSingle       IMPORT


; Subtracts two numbers and writes the result at a third location.
; Synopsis:
;	pshs	rightOpAddr
;	pshs	leftOpAddr
;	leax	result,PCR
;	lbsr	subSingleSingle
;	leas	4,s
; Preserves X.
;
subSingleSingle
	pshs	u,y,x
	ldu	#$B9B9		; unpack from X to FPA1; FPA0 = FPA1 - FPA0
	lbsr	binOpSingleSingle
	puls	x,y,u,pc




	ENDSECTION
