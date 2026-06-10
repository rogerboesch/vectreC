        INCLUDE std.inc

_printf		EXPORT
_vprintf	EXPORT

CHROUT          IMPORT
putchar_a       IMPORT
negateDWord     IMPORT
_dwtoa          IMPORT
ATOW            IMPORT
_strlen         IMPORT
unpackSingleAndConvertToASCII_hook IMPORT
singlePrecisionSize IMPORT
HEXDIG          IMPORT


	SECTION code


* void printf(const char *fmt, ...);
*
* printf(3) function.
* To redirect the output characters, store a character output
* routine address in CHROUT.
* That routine must accept the character to print in register A and
* it MUST preserve registers B, X, Y and U.
* Return value: Undefined, unlike in Standard C.
*
* About runtime minimum field width, e.g., %*d, %-*d:
* - A negative width value causes left justification.
* - So does a minus sign before the asterisk.
* - A minus sign and a negative width value cause left justification.
* - Zero-padding is not supported when using a runtime field width.
*
_printf
* ,S => Return address.
* 2,S => fmt.
* 4,S => ...
	leax    4,s             X is a va_list value
        ldd     2,s             fmt
        pshs    x,b,a           push va_list first, then fmt
        lbsr    _vprintf
        leas    4,s
        rts


* void vprintf(const char *fmt, va_list ap);
*
* Minimal vprintf(3) function.
* To redirect the output characters, store a routine pointer in CHROUT.
* That routine must accept the character to print in register A.
*
_vprintf
	PSHS	U
	LDX	4,S		format string
	LDU	6,S		variable argument pointer
	LDA	#' '		default padding char
	PSHS	A
	CLR	,-S
	CLR	,-S
* ,S = width parameter (word) (as in "%12u").
* 2,S = current padding char (byte)

mainLoop
	LDA	,X+		get char from format string
	LBEQ	return
	CMPA	#'%
	BEQ	@percentMarker
	LBSR    putchar_a	ordinary char: print it
	BRA	mainLoop
@percentMarker
	CLR	,S		clear width word
	CLR	1,S
	LDA	#' '
	STA	2,S		reinit padding char

processPercentChars		* process chars that follow '%'
	LDA	,X+		get char after '%'
	BNE	@notNUL
	LDA	#'%		string ends after '%', so print '%'
	JSR	[CHROUT,pcr]
	LBRA	return
@notNUL
	CMPA	#'-		minus?
	BNE	@notMinus
; Check for '*' instead of compile-time width.
        LDA     ,X
        CMPA    #'*
        BNE     @notMinusAsterisk
;
        LEAX    1,X             pass the asterisk in the format string
        LDD     ,U++            get signed int from stacked arguments
        BMI     @negativeRuntimeWidth
        COMA                    minus sign forces negative runtime width
        COMB
        ADDD    #1
@negativeRuntimeWidth
	STD	,S		store width in local variable
	BRA	processPercentChars	continue to process the chars following the %
@notMinusAsterisk
	LBSR	ATOW		read integer following minus sign into D
	COMA			negate D
	COMB
	ADDD	#1
	STD	,S		store negative width
	BRA	processPercentChars	continue to process the chars following the %
@notMinus
        CMPA    #'*             run-time width?
        BEQ     @runtimeWidth
	CMPA	#'0		digit that starts a width spec?
	BLO	@noWidth
	CMPA	#'9
	BHI	@noWidth
	CMPA	#'0		if number starts with 0, it specifies '0' as padding char
	BNE	@noZeroPadding
	STA	2,S		exception: this zero specifies the padding char
	BRA	@evalWidth
@noZeroPadding
	LEAX	-1,X		pointer back to 1st digit of width
@evalWidth
	LBSR	ATOW
	STD	,S		store width in local variable
	BRA	processPercentChars	continue to process the chars following the %
@runtimeWidth
        LDD     ,U++            get signed int from stacked arguments
	STD	,S		store width in local variable
	BRA	processPercentChars	continue to process the chars following the %
@noWidth
; Check for %u.
	CMPA	#'u
	BNE	@notUnsignedInt
	LDD	,U++		get 16-bit argument
;
	TST	,S		negative width? (i.e., left justification wanted?)
	BMI	leftPadU	branch if left justif.
	LBSR	PADWRD		width is in ,S, padding char in 2,S
leftPadU
	LBSR	PRNTWD		print as decimal
;
	TST	,S		negative width? (i.e., left justification wanted?)
	BPL	mainLoop	no, done
	LBSR	PADWRD		16-bit width is in ,S, padding char in 2,S
	LBRA	mainLoop
@notUnsignedInt
; Check for %d.
	CMPA	#'d
	BNE	@notSignedInt
	LDD	,U		get number to print
	BGE	@signedIntNotNegative
	COMA			number to print is < 0, so negate it
	COMB
	ADDD	#1
	PSHS	B,A
	LDD	2,S		subtract 1 one from width, because number to print is < 0
	LBSR	decAbsOfD	(but if D < 0, increment it, so that |D| is one less than before)
	STD	2,S
	PULS	A,B
*
@signedIntNotNegative
*
* Here, D is the absolute value of the number to print.
* Also, 2,S is the padding char.
* Print minus sign if word is negative AND if the padding char is '0'.
        PSHS    B
        LDB     #'0
        LBSR    printMinusIfNegativeAndCorrectPadChar
        PULS    B
;
	TST	,S		negative width? (i.e., left justification wanted?)
	BMI	@l3	        branch if left justif.
	LBSR	PADWRD		16-bit width is in ,S, padding char in 2,S
@l3
* Print minus sign if word is negative AND if the padding char is NOT '0'.
        PSHS    B
        LDB     #' '
        LBSR    printMinusIfNegativeAndCorrectPadChar
        PULS    B
;
	LBSR	PRNTWD		print as decimal
;
        LEAU    2,U             skip word in stack
;
	TST	,S		negative width? (i.e., left justification wanted?)
	LBPL	mainLoop	no, done
	LBSR	PADWRD		16-bit width is in ,S, padding char in 2,S
	LBRA	mainLoop
;
; Check for %x.
@notSignedInt
	CMPA	#'x
	BNE	@notLowerCaseX
@hexWord
	LDD	,U++		get 16-bit argument
;
	TST	,S		negative width? (i.e., left justification wanted?)
	BMI	leftPadX	branch if left justif.
	LBSR	PADHEX		16-bit width is in ,S, padding char in 2,S
leftPadX
	LBSR	PRNTWH		print as hex
;
	TST	,S		negative width? (i.e., left justification wanted?)
	LBPL	mainLoop	no, done
	LBSR	PADHEX		16-bit width is in ,S, padding char in 2,S
	LBRA	mainLoop
;
; Check for %X.
@notLowerCaseX
	CMPA	#'X
	BEQ	@hexWord
;
; Check for %p.
	CMPA	#'p
	BNE	@notPointer
	LDA	#'$		prefix pointer representation with $
	JSR	[CHROUT,pcr]
	LDD	#4		always 4 hex digits for a pointer
	STD	,S
	LDA	#'0		pad pointer with '0'
	STA	2,S
	BRA	@hexWord	do %X
@notPointer
; Check for %s.
	CMPA	#'s
	BNE	@notString
;
	LDD	,S		width of the string field
	BLT	@getStrAddr	if post-padding requested (signed branch)
;
	LDD	,U		get address of string
	LBSR	PADSTR_PRE
@getStrAddr
	LDD	,U		get address of string
	LBSR	PRINTS
;
	LDD	,S		width of the string field
	BPL	@finishedStr	if pre-padding requested: it's done, we're finished with %s
;
	LDD	,U		reload address of string
	LBSR	PADSTR_POST	do post-padding
@finishedStr
	LEAU	2,U		pass string address argument
	LBRA	mainLoop		finished with %s
@notString
; Check for %c.
	CMPA	#'c
	BNE	@notChar
        LDD     ,U++            get char in B
	TFR     B,A
	LBSR    putchar_a
	LBRA	mainLoop
@notChar
; Check for %f.
	CMPA	#'f
	BNE	@notFloat
	LBSR	printReal
        LDA	singlePrecisionSize,pcr
	LEAU	A,U		pass the float
	LBRA	mainLoop
@notFloat
; Check for %l[udxX].
	CMPA	#'l		%lu, %ld or %lx?
	LBNE	notLong
	LDA	,X+		check letter that follows %l
	CMPA	#'u
	BEQ	@ulong
	CMPA	#'d
	BEQ	@slong
	CMPA	#'x
	LBEQ	@xlong
	CMPA	#'X
	LBEQ	@xlong
	LEAX	-1,X		unknown specifier: tolerate %l as alias for %ld
	CLRA			indicates that no minus sign must be printed
@slong
* If the (big endian) dword at ,U is negative, negate it and
* print a minus sign.
	TST	,U
	BPL	@longNotNeg
	EXG	U,X
	LBSR	negateDWord
	EXG	U,X
	LDA	#'-
@longNotNeg
@ulong
*
* Here, A is '-' iff a minus sign must be printed before the number.
* Also, 16-bit width is in ,S, padding char in 2,S
*
* Call dwtoa().
	PSHS	X,A		X trashed by _dwtoa, A indicates if minus sign needed
	LEAS	-11,S		buffer for dwtoa
* Width is now in 14,S.
	LDD	,U		high word
	LDX	2,U		low word
	PSHS	X,B,A		pushes 4 bytes for _dwtoa
	LEAX	4,S		point to 11-byte buffer
	PSHS	X		pass address to _dwtoa
	LBSR	_dwtoa
	LEAS	6,S		discard arguments
	TFR	D,X		point to first digit to print
* Here, 16-bit width is in 14,S, padding char in 16,S, minus sign indicator in 11,S
*
* Print minus sign BEFORE padding IF padding char is NOT space.
	LDA	16,S		padding char
	CMPA	#' '		is space?
	BEQ	@checkPadding
	LDA	11,S		load minus sign indicator
	CMPA	#'-
	BNE	@checkPadding	no minus sign to print
	JSR	[CHROUT,pcr]	print it
*
@checkPadding
*
	TST	14,S		check high byte of width
	BMI	@l0		do not pad now if left justif.
	LBSR	@decPadding
@l0
*
* Print minus sign AFTER padding IF padding char is space.
	LDA	16,S		padding char
	CMPA	#' '		is space?
	BNE	@printBuffer
	LDA	11,S		load minus sign indicator
	CMPA	#'-
	BNE	@printBuffer	no minus sign to print
	JSR	[CHROUT,pcr]	print it
	BRA	@printBuffer
*
@printBuffer
	PSHS	X               preserve start of string to print
	BRA	@printCond
@printLoop
	JSR	[CHROUT,pcr]	print digit in A
@printCond
	LDA	,X+		load digit
	BNE	@printLoop	if not '\0'
	PULS	X		restore start of string to print (for @decPadding)
*
* Pad with spaces now if doing left justification.
	TST	14,S		check high byte of width
	BPL	@l1		do not pad now if left padding
* Here, X points to start of string to print.
	LBSR	@decPadding
@l1
	LEAS	11,S		discard buffer
	PULS	A,X		restore X (points into printf format string)
	LEAU	4,U		pass the 32-bit long
	LBRA	mainLoop
;
;
; Input: U => dword to be printed in hex.
; Output: D => number of hex digits needed to print dword (1..8).
; Preserves X and U.
;
@countHexDigitsInDWord
	CLRB
@nextDWordByte
	LDA	B,U		get byte from dword
	BNE	@nonNullByte	found 1st non-null byte
	INCB			count another null byte
	CMPB	#4		if reached 4, all bytes null
	BEQ	@oneDigitNeeded
	BRA	@nextDWordByte
@nonNullByte			; B is index (0..3) of 1st non null byte
	LSLB
	NEGB
	ADDB	#8		number of digits needed is B or B-1
	BITA	#$F0		is high nybble null?
	BNE	@highNybbleNotNull
	DECB			one less digit needed
@highNybbleNotNull	
	CLRA
	RTS
@oneDigitNeeded
	LDB	#1		A is already 0
@done
	RTS
;
;
@xlong
; Left-pad if required.
* Here, 16-bit width is in ,S, padding char in 2,S
	TST	,S				left-pad only if width >= 0
	BMI	@noHexPadding
	BSR	@countHexDigitsInDWord		uses U; result in D
	LBSR	@subAbsWidthFromD_0S		subtract abs. value of width
	BGE	@noHexPadding
	PSHS	X
	TFR	D,X				use X as upwards counter
	LDA	4,S				padding char
        BSR	@writePadChars
	PULS	X
@noHexPadding
	LDD	,U		high word
	BEQ	@highWordZero   if no digit to print
	LBSR	PRNTWH		print high word
; Print low word, padded with zeroes to a width of 4.
	LDB	#'0		padding char for PADHEX
	PSHS	B
	LDD	#4		print 2nd word as 4 digits
	PSHS	B,A		pass to PADHEX
	LDD	2,U		low word
	LBSR	PADHEX		preserves D and X
	LEAS	3,S
@highWordZero
	LDD	2,U		low word
	LBSR	PRNTWH		print D (low word)
;
	TST	,S				right-pad only if width < 0
	BPL	@noHexRightPad
	BSR	@countHexDigitsInDWord		uses U; result in D
	LBSR	@subAbsWidthFromD_0S		subtract abs. value of width
	BGE	@noHexRightPad
	PSHS	X
	TFR	D,X				use X as upwards counter
	LDA	4,S				padding char
        BSR	@writePadChars
	PULS	X
@noHexRightPad
	LEAU	4,U		pass the 32-bit long
	LBRA	mainLoop	done with %lx
;
; Input: ,S (before call) = width parameter, negative if left padding wanted.
; Output: D = original D minus the absolute value of the width.
;         CC reflects the new value in D.
;
@subAbsWidthFromD_0S
        tst     2,s            is width < 0?
        bmi     @negativeWidth_0S
        subd    2,s
        rts
@negativeWidth_0S
        addd    2,s            width < 0, so D - |width| = D - -width = D + width
        rts
;
; Prints char in A, -X times. (X is an upward counter.)
; 
@writePadChars
	JSR	[CHROUT,pcr]
	LEAX	1,X
	BNE	@writePadChars
	RTS
;
; Input: S (before call) => 11-digit buffer;
;        X => first digit to print in the 11-digit buffer;
;        16-bit width is in 14,S, padding char in 16,S, minus sign indicator in 11,S.
; Preserves X. Trashes D.
;
@decPadding
	PSHS	X
; Here, 16-bit width is in 18,S, padding char in 20,S, minus sign indicator in 15,S.
	TFR	S,D
	ADDD	#2+2+10		addr of NUL-terminating byte of buffer (written by _dwtoa)
	PSHS	X
	SUBD	,S++		D minus addr of 1st digit to print = # of digits to print
	PSHS	A
	LDA	5+11,S		get minus sign indicator
	CMPA	#'-		minus needed?
	PULS	A
	BNE	@noMinusNeeded
	ADDD	#1		count minus as 1 more char to print
@noMinusNeeded
	LBSR	@subAbsWidthFromD_18S	compare w/ width; negative result means padding needed
	BGE	@noPadding	if width matched or exceeded, no padding
	TFR	D,X		use X as upwards counter (must reach up to 0)
@decPadPrint
	LDA	20,S		padding char
	JSR	[CHROUT,pcr]
	LEAX	1,X
	BNE	@decPadPrint
@noPadding
	PULS	X,PC
;
; Input: 18,S (before call) = width parameter, negative if left padding wanted.
; Output: D = original D minus the absolute value of the width.
;         CC reflects the new value in D.
;
@subAbsWidthFromD_18S
        tst     20,s            is width < 0?
        bmi     @negativeWidth_18S
        subd    20,s
        rts
@negativeWidth_18S
        addd    20,s            width < 0, so D - |width| = D - -width = D + width
        rts


; Check for %%.
notLong	CMPA	#'%		if %%
	BNE	@unknownCode
	JSR	[CHROUT,pcr]	print '%'
	LBRA	mainLoop
@unknownCode
	PSHS	A		unknown code after '%': print '%' then the code
	LDA	#'%
	JSR	[CHROUT,pcr]
	PULS	A
	LBSR    putchar_a
	LBRA	mainLoop

return	LEAS	3,S
	PULS	U,PC


* Input: D = word to write in decimal.
*        Before call, ,S is the width.
* Output: Width word reset to 0. Padding char reset to ' '.
* Preserves A, B, X, U.
*
PADWRD	PSHS	X,B,A
	LDX	6,S		width of the number
	LBSR	absoluteX
	LEAX	-5,X		assume word in D has 5 decimal digits
	CMPD	#10
	BHS	PWD020
	LEAX	1,X		D < 10, so add one padding char
PWD020	CMPD	#100
	BHS	PWD030
	LEAX	1,X
PWD030	CMPD	#1000
	BHS	PWD040
	LEAX	1,X
PWD040	CMPD	#10000
	BHS	PWD050
	LEAX	1,X

PWD050	CMPX	#0
	BLE	PWD900		no padding if X negative or zero
PWD060	LDA	8,S		get padding char
	JSR	[CHROUT,pcr]
	LEAX	-1,X
	BNE	PWD060

PWD900	PULS	A,B,X,PC


* Input: D = address of string to write
PADSTR_PRE
	PSHS	X,B,A
	PSHS	B,A		send string address to _strlen
	LBSR	_strlen		get length of string
	STD	,S		reuse arg slot to store length
	LDD	8,S		width of the string field
	SUBD	,S		substract string length; D = number of padding chars
	BLE	PADSTR_900	if nothing to do
PADSTR_050
	TFR	D,X		use X as padding counter
PADSTR_100
	LDA	#32		use space as padding char
	JSR	[CHROUT,pcr]
	LEAX	-1,X
	BNE	PADSTR_100
PADSTR_900
	LEAS	2,S		pop arg slot
	PULS	A,B,X,PC

* Input: D = address of string to write
PADSTR_POST
	PSHS	X,B,A
	PSHS	B,A		send string address to _strlen
	LBSR	_strlen		get length of string
	STD	,S		save length
	CLRA
	CLRB
	SUBD	8,S		negated field width, which is now > 0
	SUBD	,S		subtract number of printed chars
	BLS	PADSTR_900	if nothing to do
	BRA	PADSTR_050	/* reuse previous subroutine's padding loop */


* Input: D = number to write.
*        Before call, ,S must contain 16-bit width in chars
*        and 2,S must contain padding char.
* Output: Width word reset to 0. Padding char reset to ' '.
* Preserves A, B, X, U.
*
PADHEX	PSHS	X,B,A
	LDX	6,S		width of the number
	LBSR	absoluteX
	LEAX	-4,X		assume word in D has 4 hex digits
	CMPD	#$10
	BHS	PHX020
	LEAX	1,X		D < 16, so add one padding char
PHX020	CMPD	#$100
	BHS	PHX030
	LEAX	1,X
PHX030	CMPD	#$1000
	BHS	PHX050
	LEAX	1,X
PHX050	CMPX	#0
	BLE	PHX900		no padding if X negative or zero
PHX060	LDA	8,S		get padding char
	JSR	[CHROUT,pcr]	note that this may trash A
	LEAX	-1,X
	BNE	PHX060

PHX900	CLR	6,S		clear width for next time
	CLR	7,S
	LDA	#' '		restore default padding char for next time
	STA	8,S
	PULS	A,B,X,PC


; Prints D in hex.
; Preserves D and X.
; Uses HEXDIG.
;
PRNTWH	PSHS	X,B,A
	SUBD	#0		handle special case
	BNE	PRWH10
	LDA	#'0
	JSR	[CHROUT,pcr]
	BRA	PRWH99
PRWH10	CLR	,-S		create 4-character buffer for 4 hex digits
	CLR	,-S
	CLR	,-S
	CLR	,-S
	LEAX	HEXDIG,PCR
	LSRA			get first nybble of 16-bit value to print
	LSRA
	LSRA
	LSRA
	LDA	A,X
	STA	,S
	LDA	4,S		retrieve MSB of 16-bit value of print
	ANDA	#$0F		get second nybble of 16-bit value
	LDA	A,X
	STA	1,S
	LSRB			get third nybble
	LSRB
	LSRB
	LSRB
	LDB	B,X
	STB	2,S
	LDB	5,S
	ANDB	#$0F
	LDB	B,X
	STB	3,S

	LEAX	,S		have X point to 4-char buffer
	LDB	#5		char counter
PRWH30	LDA	,X+		search for first non-'0' character
	DECB
	CMPA	#'0
	BEQ	PRWH30

	LEAX	-1,X		go back to first non-'0'
PRWH40	LDA	,X+		print the characters
	JSR	[CHROUT,pcr]
	DECB
	BNE	PRWH40

	LEAS	4,S		remove 4-char buffer

PRWH99	PULS	A,B,X,PC


* Print unsigned number in D in decimal.
PRNTWD	PSHS	X,B,A

	CLR	,-S
PRWD10	INC	,S
	SUBD	#10000
	BHS	PRWD10
	ADDD	#10000

	CLR	,-S
PRWD20	INC	,S
	SUBD	#1000
	BHS	PRWD20
	ADDD	#1000

	CLR	,-S
PRWD30	INC	,S
	SUBD	#100
	BHS	PRWD30
	ADDD	#100

	CLR	,-S
PRWD40	INC	,S
	SUBD	#10
	BHS	PRWD40
	ADDD	#10

	INCB
	PSHS	B
* All five digits are 1 more than their intended value.

	LDB	#5
	PSHS	B		loop counter
	LEAX	6,S

PRWD60	LDB	,-X		find first non-zero digit
	CMPB	#1
	BNE	PRWD80		if found (re: B is 1 more than intended value)
	DEC	,S
	BNE	PRWD60

	INC	,S		all zeroes: print one zero
	LEAX	1,X

PRWD70	LDB	,-X
PRWD80	ADDB	#'0'-1
	BSR	PRINTC
	DEC	,S
	BNE	PRWD70

	LEAS	6,S
PRWD90	PULS	A,B,X,PC


* Print the ASCII character in B.  Preserves X and D.
PRINTC	PSHS	A
	TFR	B,A
	LBSR    putchar_a
	PULS	A,PC


* Print the ASCIIZ pointed by D.  Preserves X and D.
PRINTS	PSHS	X,B,A
	TFR	D,X
	BRA	PRS020
PRS010	LBSR    putchar_a
PRS020	LDA	,X+
	BNE	PRS010
	PULS	A,B,X,PC

* Input: U => packed real number.
printReal
	PSHS	U,Y,X
	LEAU	,S		; stack frame pointer
	LEAS	-38,S		; buffer to write ASCII string to

	LDX	4,U		; address of packed number (saved U)
	PSHS	U		; save frame pointer
	LEAU	-38,U		; address of ASCII buffer
        JSR     [unpackSingleAndConvertToASCII_hook,pcr]        ; uses X and U
	PULS	U

	LEAX	-38,U		; address of ASCII buffer
@print
	LDA	,X+
	BEQ	@donePrinting
	JSR	[CHROUT,pcr]
	BRA	@print
@donePrinting
	LEAS	,U
	PULS	X,Y,U,PC

; Decrements the absolute value of D, i.e.,
; if (D < 0) ++D; else --D;
;
decAbsOfD
	CMPD	#0
	BLE	@dIsNegative
	SUBD	#1
	RTS
@dIsNegative
	ADDD	#1
	RTS

; X = |X|.
;
absoluteX	
	CMPX	#0
	BGE	@done
	EXG	X,D
	COMA
	COMB
	ADDD	#1
	EXG	X,D
@done
	RTS
	

; Prints a minus sign in the word at U is < 0.
;
printMinusIfNegative
 	TST	,U		check sign of original word to print
 	BPL	@done		if word to print < 0, print minus sign
 	PSHS	A		save absolute value of number to print
 	LDA	#'-
 	JSR	[CHROUT,pcr]
        PULS    A
@done
 	RTS

; Calls printMinusIfNegative if the padding char is equal to B.
; Input: B = char that the padding char must be for printMinusIfNegative to be called.
;        3,S (before call) = padding char
; Preserves D.
;
printMinusIfNegativeAndCorrectPadChar
        PSHS    B,A
        LDA     7,S         load padding char
        CMPA    1,S             compare w/ char in B
        BNE     @done
        LBSR    printMinusIfNegative
@done
        PULS    A,B,PC


	ENDSECTION
