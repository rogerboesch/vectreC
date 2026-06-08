#include "../src/clamp.c"
#define M 8
s16 cxin[M], cyin[M]; s8 cxo[M], cyo[M];
void run(void){ u8 i; for(i=0;i<M;i++){ cxin[i]=(s16)(i*90-300); cyin[i]=(s16)(400-i*120);}
#ifndef NOKERNEL
 clamp_vlist(cxin,cyin,cxo,cyo,M);
#endif
}
