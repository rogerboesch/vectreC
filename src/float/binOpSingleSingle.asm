	SECTION code

binOpSingleSingle	EXPORT


; Does a binary operation on two numbers and writes the result at a third location.
; Synopsis:
;	pshs	rightOpAddr
;	pshs	leftOpAddr
;	leax	result,PCR
;	lbsr	addSingleSingle		; for example
;	leas	4,s
;   [...]
; addSingleSingle
;	pshs	u,y,x
;	ldu	#colorBasicRoutine	; routine uses FPA0 & FPA1, result in FPA0
;	bsr	binOpSingleSingle
;	puls	u,x,y,pc
; Preserves X.
;
binOpSingleSingle
	ldx	12,s		; rightOpAddr
	jsr	$BC14		; unpack from X to FPA0
	ldx	10,s		; leftOpAddr
	jsr	,u		; unpack from X to FPA1; FPA0 = op(FPA0, FPA1)
	ldx	2,s		; result address
	jmp	$BC35		; pack FPA0 into X




	ENDSECTION
