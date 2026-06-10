; N.B.: Not used when using --mc6839. See mc6839-ftoa.c instead.

        INCLUDE float.inc

        SECTION code

_ftoa   EXPORT


; char *ftoa(char out[38], float f);
;
; Writes 'f' in ASCII decimal to 'out'.
; Does not precede the string with a space if 'f' is positive or zero.
;
_ftoa
	pshs	u,y		; protect against Basic

	leax	8,s		; f: number to convert to string
	flt_unpackFromXToFPA0

	ldu	6,s		; out: where to write string

        flt_convertFPA0ToASCII

	ldd	6,s		; success: return 'out'
	puls	y,u,pc


	ENDSECTION
