	SECTION code

addSingleSingle	        EXPORT

binOpSingleSingle       IMPORT


; Adds two numbers and writes the result at a third location.
; Synopsis:
;	pshs	rightOpAddr
;	pshs	leftOpAddr
;	leax	result,PCR
;	lbsr	addSingleSingle
;	leas	4,s
; Preserves X.
;
addSingleSingle
	pshs	u,y,x
	ldu	#$B9C2		; unpack from X to FPA1; FPA0 += FPA1
	lbsr	binOpSingleSingle
	puls	x,y,u,pc




	ENDSECTION
