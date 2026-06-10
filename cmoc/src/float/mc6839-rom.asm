; Float09.bin contains the 8k ROM of the MC6839 processor from Motorola.

	INCLUDE float.inc


        SECTION code


MC6839_ROM EXPORT

MC6839_ROM
        INCLUDEBIN Float09.bin
MC6839_ROM_END


        ENDSECTION
