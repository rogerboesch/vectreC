        INCLUDE std.inc

	SECTION code

_readline	EXPORT

LINBUF          IMPORT


_readline

	IFDEF THOMMO
	LEAX	LINBUF+1,pcr
	LDA #LBUFMX-1           ; room left in buffer
LOOP
	SWI                     ; system call #10: read keyboard into B
	FCB $0A
	BEQ LOOP
	CMPB #127
	BEQ DELETE
	CMPB #13
	BNE STORE
	SWI                     ; system call #2: print carriage return (in B)
	FCB $02
	CLR ,X
	LEAX	LINBUF+1,pcr
	TFR	X,D
	RTS
STORE
	TSTA
	BEQ LOOP
	DECA
	SWI                     ; system call #2: print char in B
	FCB $02
	STB ,X+
	BRA LOOP
DELETE
	CMPA #LBUFMX-1
	BEQ LOOP
	INCA
	LDB #8
	SWI
	FCB $02
	LDB #32
	SWI
	FCB $02
	LDB #8
	SWI
	FCB $02
	CLR ,-X
	BRA LOOP
	ENDC

	IFDEF THOMTO
	LEAX	LINBUF+1,pcr
	LDA #LBUFMX-1           ; room left in buffer
LOOP
	JSR $E806               ; read keyboard into B
	TSTB
	BEQ LOOP
	CMPB #127
	BEQ DELETE
	CMPB #13
	BNE STORE
	JSR $E803               ; print carriage return (in B)
	CLR ,X
	LEAX	LINBUF+1,pcr
	TFR	X,D
	RTS
STORE
	TSTA
	BEQ LOOP
	DECA
	JSR $E803               ; print char in B
	STB ,X+
	BRA LOOP
DELETE
	CMPA #LBUFMX-1
	BEQ LOOP
	INCA
	LDB #8
	JSR $E803
	LDB #32
	JSR $E803
	LDB #8
	JSR $E803
	CLR ,-X
	BRA LOOP
	ENDC


        IFDEF FLEX

LINBUF  EQU     $C080
INBUFF  EQU     $CD1B

        JSR     INBUFF      read line buffer, argv and former readlines will be overwritten!!
        LDX     #LINBUF
        LDA     #128
@readline_scan
        LDB     ,X+
        CMPB    #$0D
        BEQ     @readline_end
        DECA
        BNE     @readline_scan
        CLRB                should never happen as the end of the linebuffer should always be $0D
        RTS
@readline_end
        CLR     -1,x        put $00 as lineend instead of $0D
        LDD     #LINBUF
        RTS

        ENDC	; FLEX


	IFDEF _COCO_OR_DRAGON_BASIC_

	IFDEF _COCO_BASIC_
	JSR	$A390		CoCo Basic routine to read a line w/ cursor
	ELSE
	JSR	$B5C6		Dragon Basic routine to read a line w/ cursor
	ENDC
	LDD	#LINBUF+1	return address of 1st char
	RTS

	ENDC


        IFDEF USIM

	LEAX	LINBUF+1,pcr	+1 as in Color Basic
	LDB	#LBUFMX-1	number of bytes still available in buffer
*				(1 byte reserved for terminating NUL)
RDLN10	LDA	$FF00		check for a character from the user
	BEQ	RDLN10
	CMPA	#255		if EOF
	BEQ	CRASH
	CMPA	#10		if newline
	BEQ	RDLN90
	TSTB			room left in buffer?
	BEQ	RDLN10		if no
	STA	,X+		there is room: store the character
	DECB
	BRA	RDLN10
RDLN90	CLR	,X		terminate the string with a NUL character
	LEAX	LINBUF+1,pcr	return start of string
	TFR	X,D
	RTS

CRASH	SYNC

        ENDC


        IFDEF OS9

	pshs	y		preserve process data pointer
	leax	LINBUF+1,y	+1 as in Color Basic
	ldy	#LBUFMX-1	max num chars allowed (w/ room for final NUL)
	clra			read from stdin
	os9	I$ReadLn
	bcc	readline_ok

	clra			return null pointer to signal error
	clrb
	puls	y,pc

readline_ok
	tfr	y,d		number of bytes actually read
	cmpd	#0		empty line?
	beq	readline_empty	yes

	leax	d,x		point past last byte read
	ldb	-1,x		is last byte read carriage return?
	cmpb	#13
	bne	readline_notCR	no

	leax	-1,x		point to CR, to nullify it

readline_notCR

readline_empty
	clr	,x		terminate line with NUL
	puls	y
	leax	LINBUF+1,y	+1 as in Color Basic
	tfr	x,d		return start of string
	rts

        ENDC

        IFDEF VECTREX

* Return NULL (meaning failure) on the Vectrex, which has no keyboard.
	CLRA
	CLRB
	RTS

        ENDC

        IFDEF _CMOC_VOID_TARGET_

* Return NULL (meaning failure), because no known I/O system.
	CLRA
	CLRB
	RTS

        ENDC


	ENDSECTION
