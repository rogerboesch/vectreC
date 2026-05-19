	SECTION code

initSingleFromSignedWord	EXPORT


; Initializes the single-precision float at X with the signed word in D.
;
initSingleFromSignedWord
	pshs	u,y,x
	jsr	$B4F4			; load D (signed) into FPA0
	puls	x
	jsr	$BC35			; pack FPA0 into X
	puls	y,u,pc			; not implemented




	ENDSECTION
