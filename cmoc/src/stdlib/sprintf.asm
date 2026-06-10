	SECTION code

_sprintf        EXPORT

CHROUT          IMPORT
_printf         IMPORT
chrtomem		IMPORT
chrtomem_writer	IMPORT


* int sprintf(char *str, const char *format, ...);
*
_sprintf
	ldx	CHROUT,pcr
	stx	sprintf_oldCHROUT,pcr	preserve initial output routine address
	leax	chrtomem,PCR		install chrtomem as destination of printf()
	stx	CHROUT,pcr

	ldx	,s++			remove return address in caller of sprintf()
	stx	sprintf_retaddr,pcr	preserve it

	ldx	,s++			remove destination buffer
	stx	chrtomem_writer,pcr	make chrtomem write to destination buffer

	lbsr	_printf			print into destination buffer via chrtomem

	clr	[chrtomem_writer,pcr]	terminate resulting string

	ldx	sprintf_oldCHROUT,pcr	restore initial output routine address
	stx	CHROUT,pcr

	leas	-2,s			restore stack slot for destination buffer pointer

	jmp	[sprintf_retaddr,pcr]	return to caller of sprintf()


	ENDSECTION


        SECTION bss
        	
sprintf_retaddr	        RMB	2	used by _sprintf
sprintf_oldCHROUT	RMB	2	used by _sprintf

	ENDSECTION

