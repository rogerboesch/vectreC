	SECTION code

_strchr	EXPORT


* char *strchr(const char *s, int c);
*
_strchr:
	ldx	2,s		string
	lda	5,s		char to search for (ignore MSB)
@loop
	cmpa	,x
	beq	@found
	tst	,x+
	bne	@loop
	clra			not found: return NULL
	clrb
	rts
@found
	tfr	x,d		return address where char found
	rts


	ENDSECTION
