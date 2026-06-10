	INCLUDE float.inc

	SECTION code

initSingleFromUnsignedDWord     EXPORT

initSingleFromDWord	        IMPORT


initSingleFromUnsignedDWord
        andcc   #$FE                    ; C=0 means unsigned
        lbra    initSingleFromDWord


	ENDSECTION
