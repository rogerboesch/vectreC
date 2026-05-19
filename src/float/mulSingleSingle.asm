	SECTION code

mulSingleSingle	EXPORT

unpackXToFPA1AndMul     IMPORT
binOpSingleSingle       IMPORT


; Multiplies two numbers and writes the result at a third location.
; Synopsis:
;	pshs	rightOpAddr
;	pshs	leftOpAddr
;	leax	result,PCR
;	lbsr	mulSingleSingle
;	leas	4,s
; Preserves X.
;
mulSingleSingle
	pshs	u,y,x
	leau	unpackXToFPA1AndMul,pcr
	lbsr	binOpSingleSingle
	puls	x,y,u,pc




	ENDSECTION
