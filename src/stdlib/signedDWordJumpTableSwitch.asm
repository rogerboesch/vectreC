        SECTION code

signedDWordJumpTableSwitch      EXPORT

lookupDwordSwitchJumpTable      IMPORT
dwordSwitchJumpTableDefault     IMPORT
cmpDWordDWord                   IMPORT


; Input: X => Table of 16-bit offsets, one for each non-default case.
;        D => Address of the dword switch expression. (That dword does not get modified.)
; Table: -10,X = minimum case value (dword);
;         -6,X = maximum case value (dword);
;         -2,X = offset for default case code (word).
;
; An offset is to be added to the address of the table (in X) to obtain
; the effective address of the case code to jump to.
;
signedDWordJumpTableSwitch
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
        lbgt    dwordSwitchJumpTableDefault     ; go to default if expr > max
;
; Compare the switch expression to the minimum value.
        leax    -10,u                            point to min
        stx     2,s                              replace right arg just passed to cmpDWordDWord; keep same left arg
        lbsr    cmpDWordDWord
        lblt    dwordSwitchJumpTableDefault
        lbra    lookupDwordSwitchJumpTable


        ENDSECTION
