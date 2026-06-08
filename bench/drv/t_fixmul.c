#include "../src/fixmul.c"
void run(void){ u8 i; for(i=0;i<N;i++) vin[i]=(s16)(i*300-2000);
#ifndef NOKERNEL
 scale_q8(0x0180);
#endif
}
