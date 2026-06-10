        SECTION code

_stricmp	EXPORT

convertABToUpperCase    IMPORT
strcmpimpl				IMPORT


* int strcmp(const char *, const char *);
*
_stricmp
	leax	convertABToUpperCase,PCR
	lbra	strcmpimpl


        ENDSECTION
