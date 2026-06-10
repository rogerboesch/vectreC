	SECTION code


; OS-9: Nothing generated. See os9-sbrkmax.c.
	IFNDEF OS9


_sbrkmax        EXPORT
end_of_sbrk_mem IMPORT
program_break   IMPORT


* size_t sbrkmax(void);
*
* Returns (in D) the maximum number of bytes that can be successfully
* asked of sbrk().
*
_sbrkmax

	ldd	end_of_sbrk_mem,pcr
	subd	program_break,pcr
	bhs     @sbrkmax_non_neg
* The program break is after the stack space. Not supported by sbrk().
	clra
	clrb
@sbrkmax_non_neg
	rts


	ENDC


	ENDSECTION
