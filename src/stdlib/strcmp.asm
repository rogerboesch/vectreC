		SECTION code

_strcmp		EXPORT

strcmpimpl	IMPORT


* int strcmp(const char *, const char *);
*
_strcmp
		leax	noTransform,PCR
		lbra	strcmpimpl

noTransform
		rts


		ENDSECTION
