        INCLUDE float.inc

	SECTION code

_strtof	EXPORT

CHARAD  IMPORT
GETCCH  IMPORT


; float strtof(char *nptr, char **endptr);
;
; The string must have at most 255 characters (before the null terminator).
; Caution: Passing a excessive value will make Basic fail with an OV ERROR.
;
_strtof

	pshs	u,y

	ldd	CHARAD		; save interpreter's input pointer
	pshs	b,a
	ldx	10,s		; nptr: string to parse
	stx	CHARAD		; point interpreter to caller's string

	jsr	GETCCH		; get current char
	jsr	$BD12		; result in FPA0; trashes FPA1

	ldx	8,s		; address of returned float (hidden parameter)
	jsr	$BC35		; pack FPA0 into X

	ldd	CHARAD		; get address in nptr where parsing stopped
	std	[12,s]		; store at *endptr

	puls	a,b
	std	CHARAD		; restore interpreter's input pointer

	puls	y,u,pc




	ENDSECTION
