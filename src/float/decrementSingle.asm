	SECTION code

decrementSingle	EXPORT


; Subtracts one 1 from the packed single-precision float at X.
; Preserves X.
;
decrementSingle
	pshs	u,y,x
	jsr	$BC14		; unpack into Basic's FPA0 (preserves X)
	leax	packedMinus1,PCR
	jsr	$B9C2		; add number at X to FPA0 (trashes X)
	ldx	,s		; retrieve original number address
	jsr	$BC35		; pack FPA0 at X
	puls	x,y,u,pc
packedMinus1
	fcb	$81		; packed -1.0
	fdb	$8000
	fdb	0




	ENDSECTION
