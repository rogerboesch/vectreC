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
	jsr	$BC14		; load FPA0 from X

	ldu	6,s		; out: where to write string

	ldb	FP0SGN		; get sign of 'f'
	bpl	@positive
	lda	#'-'
	sta	,u+		; output minus sign
	bra	@signDone
@positive
	lda	#' '
@signDone
	jsr	$BDE6		; let Basic do rest of conversion

	ldd	6,s		; success: return 'out'
	puls	y,u,pc


	ENDSECTION
