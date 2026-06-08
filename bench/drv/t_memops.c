#include "../src/memops.c"
u8 mdst[40], msrc[40];
void run(void){ u8 i; for(i=0;i<40;i++) msrc[i]=(u8)(i*3);
#ifndef NOKERNEL
 buf_copy(mdst,msrc,40); buf_set(mdst,0xAA,40);
#endif
}
