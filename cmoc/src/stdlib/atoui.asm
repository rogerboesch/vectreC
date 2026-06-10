	SECTION code

_atoui	EXPORT

passLeadingWhiteSpaceChars      IMPORT
ATOW    IMPORT


* unsigned atoui(char *s);
_atoui
        ldx     2,s             argument 's'
        lbsr    passLeadingWhiteSpaceChars
        lbra    ATOW


	ENDSECTION
