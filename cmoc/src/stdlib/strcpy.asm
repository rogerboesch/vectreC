	SECTION code

_strcpy	EXPORT


* char *strcpy(char *dest, const char *src);
* Returns dest.
*
_strcpy
	pshs	u
	ldx	4,s		dest
	ldu	6,s		src

@loop
	lda	,u+
	sta	,x+
	bne	@loop

	ldd	4,s		destination string
	puls	u,pc


	ENDSECTION
