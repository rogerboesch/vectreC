; Floating-point assembly language macros.
;
; In an LWASM macro, as of LWTOOLS 4.18, a label must be followed by 'equ *';
; it cannot be alone on its line.

        IFDEF OS9                       ;;OS9 (lines with ";;OS9" are not copied to float-os9.inc)
        INCLUDE float-os9.inc           ;;OS9
        ELSE                            ;;OS9


; Section that implements a common programming interface using
; Extended Color Basic's floating-point routines.
;
        IFDEF _CMOC_MS_FLOAT_

EXPBIAS EQU     128             ; bias applied to a stored exponent

FP0ADDR	EQU	$004F
FP1ADDR	EQU	$005C

; Fields of Color Basic's first two (unpacked) float point accumulators.
FP0EXP	EQU	FP0ADDR
FP0MAN	EQU	FP0ADDR+1
FP0SGN	EQU	FP0ADDR+5
FP1EXP	EQU	FP1ADDR
FP1MAN	EQU	FP1ADDR+1
FP1SGN	EQU	FP1ADDR+5

VALTYP	EQU	$0006		; variable type (0 = numeric)
RESSGN	EQU	$0062		; sign of result of operation
FPSBYT  EQU     $0063           ; floating point sub byte

; Machine-dependent addresses in MS Basic.

        IFDEF _COCO_BASIC_

; Various routines from Color Basic with most labels coming from the Unravelled books.
;
GIVABF                  EQU     $B4F4
LBC14                   EQU     $BC14
LBC35                   EQU     $BC35
LBB2F                   EQU     $BB2F
LB9C2                   EQU     $B9C2
LB9C5                   EQU     $B9C5
LB9BC                   EQU     $B9BC
LBA18                   EQU     $BA18
CmdMultiply             EQU     $BACC           ; label from DragonROM git repo
LBB91                   EQU     $BB91
LBC96                   EQU     $BC96
LBDE6                   EQU     $BDE6           ; original label


; Routine that converts an ASCII decimal string at X into FPA0.
; The character at X must be in A. The carry must be set iff A is a numeric character.
; Trashes FPA1.
;
convertStrToFlt         EQU     $BD12

        ENDC

        IFDEF DRAGON

GIVABF                  EQU     $8C37
LBC14                   EQU     $93BF
LBC35                   EQU     $93E0
LBB2F                   EQU     $92DA
LB9C2                   EQU     $910B
LB9C5                   EQU     $910E
LB9BC                   EQU     $9105
LBA18                   EQU     $9161
CmdMultiply             EQU     $9275
LBB91                   EQU     $933C
LBC96                   EQU     $9441
LBDE6                   EQU     $9594

convertStrToFlt         EQU     $94BD

        ENDC


; Interface to a set of floating-point routines.
; Based on Extended Color Basic.

; May trash D, X, FPA1.
;
flt_loadSignedDIntoFPA0 MACRO
        jsr     GIVABF
        ENDM


; Preserves X and U.
;
flt_unpackFromXToFPA0 MACRO
        pshs    x
        jsr     LBC14
        puls    x
        ENDM


; Trashes A, U. Preserves X.
;
flt_packFPA0ToX MACRO
        jsr     LBC35
        ENDM


flt_unpackFromXToFPA1 MACRO
        jsr     LBB2F
        ENDM


; Adds the packed float at X to FPA0, leaves the sum in FPA0.
; Trashes X and FPA1.
; Preserves U.
;
flt_addNumberAtXToFPA0 MACRO
        jsr     LB9C2
        ENDM


; FPA0 = FPA1 + FPA0
; Input: FPA0, FPA1.
; Output: Result in FPA0.
; Allowed to trash FPA1.
;
flt_addFPA1ToFPA0 MACRO
	lda     FP1EXP		; load exponent of FPA1
	ldb     FP0EXP		; load exponent of FPA0, set N and Z
        jsr     LB9C5
        ENDM


; FPA0 = FPA1 - FPA0
; Input: FPA0, FPA1.
; Output: Result in FPA0.
; Allowed to trash FPA1.
;
flt_subFPA0FromFPA1 MACRO
        lda     FP1EXP
        ldb     FP0EXP          ; sets N and Z
        jsr     LB9BC
        ENDM


; Input: C is 1 if the value in FPA0 is non-negative.
; If C is 0, the sign is inverted and the mantissa is negated (as per 32-bit two's complement)
; before the normalization is done.
; Branches to the Color Basic OV ERROR routine upon overflow.
; Trashes D, X.
;
flt_normalizeFPA0 MACRO
        jsr     LBA18
        ENDM


; FPA0 *= FPA1.
;
flt_mulFPA0ByFPA1 MACRO
        lda     FP1EXP
        ldb     FP0EXP          ; sets N and Z
        jsr     CmdMultiply
        ENDM


; Input: FPA0 = divisor, FPA1 = dividend.
; Output: FPA0 = quotient.
; Allowed to trash FPA1.
;
flt_divFPA1ByFPA0 MACRO
        lda     FP1EXP
        ldb     FP0EXP          ; sets N and Z
        jsr     LBB91
        ENDM


; Compares FPA0 to packed number at X.
; Puts 0 in B if equals, +1 in B if FPA0 > *X, -1 in B if FPA0 < *X.
; Sets N and Z according to B. Sets V to 0.
; Trashes A. Allowed to trash X.
;
flt_compareFPA0ToX MACRO
        jsr     LBC96
        ENDM


; Does not precede the string with a space if FPA0 is positive or zero.
; Input: FPA0 contains the number to be converted.
;        U => ASCII buffer to write to.
; Trashes: A, B, X, FPA0.
; Based on Color Basic code at $BDDC.
;
flt_convertFPA0ToASCII MACRO
	tst	FP0SGN		; get sign of FPA0
	bpl	@positive
	lda	#'-'
	sta	,u+		; output minus sign
	bra	@signDone
@positive equ *
	lda	#' '
@signDone equ *
	jsr	LBDE6		; let Basic do rest of conversion
        ENDM


; Set value type to numeric -- only applicable under Color Basic.
;
flt_clrVALTYP MACRO
        clr     VALTYP
        ENDM


; Clear sub byte -- only applicable under Color Basic.
;
flt_clrFPSBYT MACRO
	clr	FPSBYT
        ENDM


; Compute sign of result from FP0SGN and FP1SGN -- only applicable under Color Basic.
; Trashes B.
;
flt_computeResultSign MACRO
	ldb     FP0SGN
	eorb    FP1SGN
	stb     RESSGN
        ENDM


; Copy accumulator 0 to accumulator 1.
;
flt_copyFPA0ToFPA1 MACRO
	ldd	FP0ADDR
	std	FP1ADDR
	ldd	FP0ADDR+2
	std	FP1ADDR+2
	ldd	FP0ADDR+4
	std	FP1ADDR+4
        ENDM


; Only changes A.
; Output: A is the biased exponent of FPA0, and Z reflects this byte value.
;
flt_loadAFromFPA0BiasedExp MACRO
        lda     FP0EXP
        ENDM


; Does not change B. Affects CC.
;
flt_storeBInFPA0BiasedExp MACRO
	stb	FP0EXP
        ENDM


; Does not change any data registers.
; Resulting CC is not significant.
;
flt_clearFPA0Sign MACRO
	clr	FP0SGN
        ENDM


; Does not change any data registers.
; Resulting CC is not significant.
;
flt_invertFPA0Sign MACRO
	com	FP0SGN
        ENDM


; Does not change any data registers.
; Affects N.
; 
flt_testFPA0Sign MACRO
	tst	FP0SGN
        ENDM


flt_invertFPA1Sign MACRO
	com	FP1SGN
        ENDM


; Resulting CC is not significant.
; 
flt_loadBFromTopFPA0MantissaByte MACRO
        ldb     FP0MAN
        ENDM


        ENDC    ; _CMOC_MS_FLOAT_


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


; Section that implements a common programming interface using
; CMOC's own floating-point routines.
;
; CAUTION: See the _CMOC_MS_FLOAT_ section above for the expected behavior of each macro.
;
; These routines are not re-entrant, namely because FPA0 and FPA1
; are global variables.
;
        IFDEF _CMOC_NATIVE_FLOAT_

EXPBIAS EQU     128             ; bias applied to a stored exponent

; Unpacked float point accumulators.
;
FP0ADDR         IMPORT
FP0EXP          IMPORT
FP0MAN          IMPORT
FP0SGN          IMPORT
FP1ADDR         IMPORT
FP1EXP          IMPORT
FP1MAN          IMPORT
FP1SGN          IMPORT


_floatable_pack                         IMPORT
_floatable_unpack                       IMPORT
_floatable_toDecimal                    IMPORT
_floatable_addUFloat40                  IMPORT
_floatable_subUFloat40                  IMPORT
_floatable_mulUFloat40                  IMPORT
_floatable_divUFloat40                  IMPORT
_floatable_cmpUFloat40                  IMPORT
_floatable_normalizeUFloat40            IMPORT
_floatable_setUFloat40ToSignedInt       IMPORT


flt_loadSignedDIntoFPA0 MACRO
; Call floatable_setUFloat40ToSignedInt(UFloat40 *dest, signed int n).
        pshs    d                       ; pass D for 'n'
        leax    FP0ADDR,pcr
        pshs    x                       ; pass address of FPA0 for 'dest'
        lbsr    _floatable_setUFloat40ToSignedInt
        leas    4,s
        ENDM

flt_unpackFromXToFPA0 MACRO
; Call floatable_unpack(UFloat40 *dest, const PFloat40 *src).
        pshs    x                       ; preserve X
        pshs    x                       ; pass X for 'src'
        leax    FP0ADDR,pcr
        pshs    x                       ; pass address of FPA0 for 'dest'
        lbsr    _floatable_unpack
        leas    4,s                     ; pop parameters
        puls    x                       ; restore X
        ENDM

flt_packFPA0ToX MACRO
; Call floatable_pack(PFloat40 *dest, const UFloat40 *src).
        pshs    x                       ; preserve X
        leax    FP0ADDR,pcr
        pshs    x                       ; pass address of FPA0 for 'src'
        ldx     2,s                     ; get preserved X, which is the destination
        pshs    X                       ; pass it for 'dest'
        lbsr    _floatable_pack
        leas    4,s                     ; pop parameters
        puls    x                       ; restore X
        ENDM

flt_unpackFromXToFPA1 MACRO
; Call floatable_unpack(UFloat40 *dest, const PFloat40 *src).
        pshs    x                       ; preserve X
        pshs    x                       ; pass X for 'src'
        leax    FP1ADDR,pcr
        pshs    x                       ; pass address of FPA1 for 'dest'
        lbsr    _floatable_unpack
        leas    4,s                     ; pop parameters
        puls    x                       ; restore X
        ENDM

flt_addNumberAtXToFPA0 MACRO
; Call floatable_unpack(UFloat40 *dest, const PFloat40 *src).
        pshs    x                       ; pass X for 'src'
        leax    FP1ADDR,pcr
        pshs    x                       ; pass address of FPA1 for 'dest'
        lbsr    _floatable_unpack
; Do not pop the 4 param bytes: reuse them in following call.
; Call floatable_addUFloat40(UFloat40 *fp0, const UFloat40 *fp1).
        leax    FP1ADDR,pcr
        stx     2,s                     ; pass for fp1
        leax    FP0ADDR,pcr
        stx     ,s                      ; pass for fp0
        lbsr    _floatable_addUFloat40
        leas    4,s
        ENDM

flt_addFPA1ToFPA0 MACRO
; Call floatable_addUFloat40(UFloat40 *fp0, const UFloat40 *fp1).
        leax    FP1ADDR,pcr
        pshs    x
        leax    FP0ADDR,pcr
        pshs    x
        lbsr    _floatable_addUFloat40
        leas    4,s
        ENDM

; FPA0 = FPA1 - FPA0
;
flt_subFPA0FromFPA1 MACRO
; Ignore exponents in A and B.
; Do FPA0 - FPA1, then negate result.
; Call floatable_subUFloat40(UFloat40 *dest, const UFloat40 *other).
        leax    FP1ADDR,pcr
        pshs    x                       ; pass FPA1 for 'other'
        leax    FP0ADDR,pcr
        pshs    x                       ; pass FPA0 for 'dest'
        lbsr    _floatable_subUFloat40  ; get result in FPA0
        leas    4,s
        leax    FP0ADDR,pcr
        tst     ,x                      ; check exponent of result
        beq     @done                   ; if result is zero, nothing to negate
        com     5,x                     ; invert sign byte
@done equ *
        ENDM

flt_normalizeFPA0 MACRO
; Call floatable_normalizeUFloat40(UFloat40 *fp, BOOL negateFirst).
        tfr     cc,b            ; put carry in B
        andb    #1              ; only keep carry bit
        eorb    #1
        clra
        pshs    d               ; pass BOOL for 'negateFirst'
        leax    FP0ADDR,pcr
        pshs    x               ; pass address of FPA0 for 'fp'
        lbsr    _floatable_normalizeUFloat40
        leas    4,s
        ENDM

flt_mulFPA0ByFPA1 MACRO
; Call floatable_mulUFloat40(UFloat40 *fp0, const UFloat40 *fp1).
        leax    FP1ADDR,pcr
        pshs    x                       ; pass address of FPA1 for 'fp1'
        leax    FP0ADDR,pcr
        pshs    x                       ; pass address of FPA0 for 'fp0'
        lbsr    _floatable_mulUFloat40
        leas    4,s
        ENDM

; FPA0 = FPA1 / FPA0.
;
flt_divFPA1ByFPA0 MACRO
; Call floatable_divUFloat40(UFloat40 *dividend, const UFloat40 *divisor).
        leax    FP0ADDR,pcr
        pshs    x                       ; pass address of FPA0 for 'divisor'
        leax    FP1ADDR,pcr
        pshs    x                       ; pass address of FPA1 for 'dividend'
        lbsr    _floatable_divUFloat40  ; quotient left in FPA1
        leas    4,s
; Copy FPA1 to FPA0.
        ldd     FP1ADDR,pcr
        std     FP0ADDR,pcr
        ldd     FP1ADDR+2,pcr
        std     FP0ADDR+2,pcr
        ldd     FP1ADDR+4,pcr
        std     FP0ADDR+4,pcr
        ENDM

flt_compareFPA0ToX MACRO
; Call floatable_unpack(UFloat40 *dest, const PFloat40 *src) to unpack *X.
        pshs    x                       ; pass X for 'src'
        leax    FP1ADDR,pcr
        pshs    x                       ; pass address of FPA1 for 'dest'
        lbsr    _floatable_unpack
; Do not pop the 4 param bytes: reuse them in following call.
; Call char floatable_cmpUFloat40(const UFloat40 *left, const UFloat40 *right) to compare FPA0 and FPA1.
        leax    FP1ADDR,pcr
        stx     2,s                     ; pass address of FPA1 for 'right'
        leax    FP0ADDR,pcr
        stx     ,s                      ; pass address of FPA0 for 'left'
        lbsr    _floatable_cmpUFloat40
        leas    4,s
        tstb
        ENDM

flt_convertFPA0ToASCII MACRO
; Call floatable_toDecimal(char dest[DEC_ACC_WIDTH + 18], const UFloat40 *num)
        leax    FP0ADDR,pcr             ; pass address of FPA0 as 'num' parameter
        pshs    x
        pshs    u                       ; pass output buffer address as 'dest' parameter
        lbsr    _floatable_toDecimal
        leas    4,s                     ; pop parameters
        ENDM

; Nothing to do when not under Color Basic.
;
flt_clrVALTYP MACRO
        ENDM

; Nothing to do when not under Color Basic.
;
flt_clrFPSBYT MACRO
        ENDM

; Nothing to do when not under Color Basic.
; B should be considered trashed.
;
flt_computeResultSign MACRO
        ENDM


flt_copyFPA0ToFPA1 MACRO
	ldd	FP0ADDR,pcr
	std	FP1ADDR,pcr
	ldd	FP0ADDR+2,pcr
	std	FP1ADDR+2,pcr
	ldd	FP0ADDR+4,pcr
	std	FP1ADDR+4,pcr
        ENDM


flt_loadAFromFPA0BiasedExp MACRO
        lda     FP0EXP,pcr
        ENDM


flt_storeBInFPA0BiasedExp MACRO
	stb	FP0EXP,pcr
        ENDM


flt_clearFPA0Sign MACRO
	clr	FP0SGN,pcr
        ENDM


flt_invertFPA0Sign MACRO
	com	FP0SGN,pcr
        ENDM


flt_testFPA0Sign MACRO
	tst	FP0SGN,pcr
        ENDM


flt_invertFPA1Sign MACRO
	com	FP1SGN,pcr
        ENDM


flt_loadBFromTopFPA0MantissaByte MACRO
        ldb     FP0MAN,pcr
        ENDM


        ENDC    ; _CMOC_NATIVE_FLOAT_


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


; Section that implements a common programming interface using
; the Motorola MC6839 floating point ROM.
;
; CAUTION: See the _CMOC_MS_FLOAT_ section above for the expected behavior of each macro.
;
; These routines are not re-entrant, namely because FPA0 and FPA1
; are global variables.
;
; See "MC6839 Floating-point ROM Datasheet.pdf", page 4 ("Floating Point Formats").
;
        IFDEF _CMOC_MC6839_

; MC6839 floating point control block.
fpcb            STRUCT
control                 rmb 1
trap_enable             rmb 1
status                  rmb 1
secondary_status        rmb 1
trap_routine            rmb 2
                ENDSTRUCT

EXPBIAS         EQU     127             ; bias applied to a stored exponent
MC6839_ROM      IMPORT                  ; address where the MC6839 ROM is loaded
FPREG           EQU     MC6839_ROM+$3D  ; MC6839 entry point for register-accepting routines
FPCB_SINGLE     IMPORT                  ; type descriptor for single-precision (40-bit) floats (see fpcb struct above)

; MC6839 operation codes.
FADD            EQU     $00
FSUB            EQU     $02
FMUL            EQU     $04
FDIV            EQU     $06
FREM            EQU     $08
FSQRT           EQU     $12
FINT            EQU     $14
FFIXS           EQU     $16
FFIXD           EQU     $18
FAB             EQU     $1E
FNEG            EQU     $20
FFLTS           EQU     $24
FFLTD           EQU     $26
FCMP            EQU     $8A
FTCMP           EQU     $CC
FPCMP           EQU     $8E
FTPCMP          EQU     $D0
FMOV            EQU     $9A
BINDEC          EQU     $1C
DECBIN          EQU     $22

; Unpacked float point accumulators.
;
FP0ADDR         IMPORT
FP1ADDR         IMPORT


flt_loadSignedDIntoFPA0 MACRO
initSingleFromSignedWord IMPORT
        leax    FP0ADDR,pcr
        lbsr    initSingleFromSignedWord
        ENDM

flt_unpackFromXToFPA0 MACRO
        ldd     ,x
        std     FP0ADDR,pcr
        ldd     2,x
        std     FP0ADDR+2,pcr
        ENDM

flt_packFPA0ToX MACRO
        ldd     FP0ADDR,pcr
        std     ,x
        ldd     FP0ADDR+2,pcr
        std     2,x
        ENDM

flt_unpackFromXToFPA1 MACRO
        ldd     ,x
        std     FP1ADDR,pcr
        ldd     2,x
        std     FP1ADDR+2,pcr
        ENDM

flt_addNumberAtXToFPA0 MACRO
addNumberAtXToFPA0 IMPORT
        lbsr    addNumberAtXToFPA0
        ENDM

flt_addFPA1ToFPA0 MACRO
addFPA1ToFPA0 IMPORT
        lbsr    addFPA1ToFPA0
        ENDM

flt_subFPA0FromFPA1 MACRO
subFPA0FromFPA1 IMPORT
        lbsr    subFPA0FromFPA1
        ENDM

flt_mulFPA0ByFPA1 MACRO
mulFPA0ByFPA1 IMPORT
        lbsr    mulFPA0ByFPA1
        ENDM

flt_divFPA1ByFPA0 MACRO
divFPA1ByFPA0 IMPORT
        lbsr    divFPA1ByFPA0
        ENDM

flt_compareFPA0ToX MACRO
compareFPA0ToX IMPORT
        lbsr    compareFPA0ToX
        ENDM

_ftoa                   IMPORT
push4ByteStruct         IMPORT

flt_convertFPA0ToASCII MACRO
; Call ftoa(char out[], float f) in float/mc6839-ftoa.c.
        leax    FP0ADDR,pcr             ; pass address of FPA0 as 'f' parameter
        leas    -4,s
        lbsr    push4ByteStruct         ; push float pointed to by X
        pshs    u                       ; pass output buffer address as 'dest' parameter
        lbsr    _ftoa                   ; preserves U
        leas    2+4,s                   ; pop parameters
        ENDM


; Nothing to do when not under Color Basic.
; B should be considered trashed.
;
flt_computeResultSign MACRO
        ENDM


; Copy 32 bits.
;
flt_copyFPA0ToFPA1 MACRO
	ldd	FP0ADDR,pcr
	std	FP1ADDR,pcr
	ldd	FP0ADDR+2,pcr
	std	FP1ADDR+2,pcr
        ENDM


flt_loadAFromFPA0BiasedExp MACRO
loadAFromFPA0BiasedExp IMPORT
        lbsr    loadAFromFPA0BiasedExp
        ENDM


flt_storeBInFPA0BiasedExp MACRO
storeBInFPA0BiasedExp IMPORT
        lbsr    storeBInFPA0BiasedExp
        ENDM


flt_clearFPA0Sign MACRO
        pshs    a
        lda     FP0ADDR,pcr
        anda    #$7F
        sta     FP0ADDR,pcr
        puls    a
        ENDM


flt_invertFPA0Sign MACRO
        pshs    a
        lda     FP0ADDR,pcr
        eora    #$80
        sta     FP0ADDR,pcr
        puls    a
        ENDM


flt_testFPA0Sign MACRO
        tst     FP0ADDR,pcr
        ENDM


flt_invertFPA1Sign MACRO
        pshs    a
        lda     FP1ADDR,pcr
        eora    #$80
        sta     FP1ADDR,pcr
        puls    a
        ENDM


        ENDC    ; _CMOC_MC6839_


        ENDC    ;;OS9
