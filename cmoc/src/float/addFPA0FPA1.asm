	INCLUDE float.inc

	SECTION code

addFPA0FPA1	EXPORT


* Routine that finishes work started by other routines, e.g., addIntSingle.
* Input: FPA1 = number to add to FPA0.
*        Pushed argument = address of resulting packed float.
* Output: Sum (packed single) stored at X.
* Trashes FPA0.
* Preserves X.
*
addFPA0FPA1
	flt_addFPA1ToFPA0

	ldx	,s		; result
	flt_packFPA0ToX
	puls	x,y,u,pc




	ENDSECTION
