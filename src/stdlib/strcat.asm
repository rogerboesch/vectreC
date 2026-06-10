	SECTION code

_strcat	EXPORT

_strcpy IMPORT


* char *strcat(char *dest, const char *src);
* Returns dest.
*
_strcat
	ldx	4,s		src
	pshs	x		pass it to _strcpy

	ldx	4,s		dest (because of preceding pshs)
@loop
	tst	,x+
	bne	@loop

	leax	-1,x		point back to '\0'
	pshs	x		pass as destination string to _strcpy

	lbsr	_strcpy

	leas	4,s		dispose of _strcpy arguments
	ldd	2,s		return dest
	rts



	ENDSECTION
