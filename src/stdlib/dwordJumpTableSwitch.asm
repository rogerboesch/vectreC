        SECTION code

lookupDwordSwitchJumpTable      EXPORT
dwordSwitchJumpTableDefault     EXPORT

signedDWordJumpTableSwitch      IMPORT
unsignedDWordJumpTableSwitch    IMPORT


subDWordDWord   IMPORT


lookupDwordSwitchJumpTable
;
; Subtract the maximum value from the switch expression.
; (Must subtract separately from cmpDWordDWord b/c subDWordDWord does not set CC properly.)
        leax    4,s             ; point to temp dword, which will become subtract result
        lbsr    subDWordDWord   ; reuse stacked argument just used by cmpDWordDWord; trashes D
        leas    4,s             ; discard the 2 pointers passed to subDWordDWord
        ldd     2,s             ; load low word of substract result (high word presumed zero)
;
; D is now the zero-based index of the table entry we want.
        lslb                    ; 2 bytes per address in table
        rola
        ldd     d,u             ; get offset from table entry
        bra     @jump
;
dwordSwitchJumpTableDefault
        leas    4,s             ; discard the 2 pointers passed to cmpDWordDWord
        ldd     -2,u            ; get offset of default routine
;
; D is now an offset from the table pointer which is in U.
@jump
        leax    d,u             ; add table address to offset to get absolute address to jump to
        leas    4,s             ; discard temp dword
        puls    u               ; restore caller's register(s)
        jmp     ,x              ; jump to switch case or default


        ENDSECTION
