	INCLUDE float.inc

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
	leau    @addNumberAtXToFPA0,PCR
	lbsr	binOpSingleSingle
	puls	x,y,u,pc
;
@addNumberAtXToFPA0
	flt_addNumberAtXToFPA0
	rts


	ENDSECTION
