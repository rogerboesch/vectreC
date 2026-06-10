        SECTION code

unsignedDWordJumpTableSwitch    EXPORT

lookupDwordSwitchJumpTable      IMPORT
dwordSwitchJumpTableDefault     IMPORT
cmpDWordDWord                   IMPORT


; Similar to signedDWordJumpTableSwitch.
;
unsignedDWordJumpTableSwitch
        pshs    u
        leas    -4,s
;
; 0..3,S: Temporary dword.
;
        leau    ,x                              ; point U to the table
;
; Compare the switch expression to the maximum value.
        leax    -6,u                            ; point to max
        pshs    x                               ; pass max ptr to cmpDWordDWord (right arg)
        pshs    b,a                             ; pass expr ptr to cmpDWordDWord (left arg)
        lbsr    cmpDWordDWord                   ; preserves X, Y, U; trashes D
        lbhi    dwordSwitchJumpTableDefault     ; go to default if expr > max
;
; Compare the switch expression to the minimum value.
        leax    -10,u                           ; point to min
        stx     2,s                             ; replace right arg just passed to cmpDWordDWord; keep same left arg
        lbsr    cmpDWordDWord
        lblo    dwordSwitchJumpTableDefault
        lbra    lookupDwordSwitchJumpTable


        ENDSECTION
