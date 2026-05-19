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
FPCARY  EQU     $005B           ; floating point carry byte
RESSGN	EQU	$0062		; sign of result of operation
FPSBYT  EQU     $0063           ; floating point sub byte

GETCCH	EQU	$00A5
CHARAD	EQU	$00A6		; interpreter's input pointer
COEFCT	EQU	$0055		; polynomial coefficient counter
