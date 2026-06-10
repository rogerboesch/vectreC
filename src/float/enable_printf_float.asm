        INCLUDE float.inc

        SECTION code

_enable_printf_float EXPORT

unpackSingleAndConvertToASCII IMPORT
unpackSingleAndConvertToASCII_hook IMPORT


; void enable_printf_float(void);
;
; For a program that uses --mc6839, i.e., not the Color Basic float routines,
; the reference to unpackSingleAndConvertToASCII will cause the linker to bring
; in additional object files to provide the needed code.
;
; A program that does not use floats should omit using enable_printf_float(),
; so that this .asm file does not get brought into the program, and thus
; no reference to unpackSingleAndConvertToASCII is made, and no MC6839
; support code gets brought in.
;
_enable_printf_float
        leax    unpackSingleAndConvertToASCII,PCR       ; PCR in caps b/c ref to code
        stx     unpackSingleAndConvertToASCII_hook,pcr
        rts


	ENDSECTION
