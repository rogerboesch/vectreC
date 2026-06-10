	INCLUDE std.inc

	SECTION code

_delay	EXPORT


	IFDEF THOMMO
numDelayIters EQU $800
	ENDC
	IFDEF THOMTO
numDelayIters EQU $800
	ENDC
	IFDEF FLEX
numDelayIters EQU $AE6		; based on 1.3398 MHz CPU frequency
	ENDC


* Wait for a number of ticks (1/60 second) which is given on the stack.
_delay
	IFDEF numDelayIters
	LDX 2,S
	BEQ @EXIT
	PSHS Y
	LDY #numDelayIters
@LOOP
	LEAY -1,Y		; 5 cycles
	BNE @LOOP		; 3 cycles
	LDY #numDelayIters
	LEAX -1,X
	BNE @LOOP
	PULS Y
@EXIT
	RTS
	ENDC


	IFDEF _COCO_OR_DRAGON_BASIC_

	LDD	2,S		number of ticks to wait
	BEQ	DELAY9
	ADDD	TIMER		value that TIMER must reach
DELAY1	CMPD	TIMER
	BNE	DELAY1
DELAY9	RTS

	ENDC


	IFDEF USIM

	LDD	2,S		number of ticks to wait
	STD	$FF02		ask simulator to wait
	RTS

	ENDC


	IFDEF OS9

	ldx	2,s
	os9	F$Sleep
	rts

	ENDC


	IFDEF VECTREX

	RTS

	ENDC


	IFDEF _CMOC_VOID_TARGET_

	RTS

	ENDC


	ENDSECTION
