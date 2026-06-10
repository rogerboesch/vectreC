#include "coco.h"


void
setHighSpeed(BOOL fast)
{
    asm
    {
        ldx     #65494
        tst     :fast
        beq     @slow
        leax    1,x
@slow:
#ifdef _COCO_BASIC_
        ldd     $FFF8       ; IRQ vector
        cmpd    #$FEF7      ; CoCo 3?
        bne     @notCoCo3   ; branch if not
        leax    2,x
@notCoCo3:
#endif
        clr     ,x
    }
}
