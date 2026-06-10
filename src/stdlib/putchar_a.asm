        SECTION code

_putchar        EXPORT
putchar_a       EXPORT

CHROUT          IMPORT


* void putchar(byte c)
*
* Converts LF ($0A) into CR ($0D) before calling CHROUT.
*
_putchar
        lda     3,s

* putchar_a should be called by the other routines (printf, etc.)
* whenever the character to be printed might be a newline.
*
putchar_a

    IFDEF _COCO_OR_DRAGON_BASIC_
        cmpa    #10             C linefeed?
        bne     @done
        lda     #13             CHROUT does newline on char 13
    ENDC
    IFDEF OS9
        cmpa    #10             C linefeed?
        bne     @done
        lda     #13             for I$WritLn
    ENDC
    IFDEF FLEX
        cmpa    #10             C linefeed?
        bne     @done
        jsr     [CHROUT,pcr]    FLEX sequence is {10, 13}
        lda     #13
    ENDC
@done
        jmp     [CHROUT,pcr]




        ENDSECTION
