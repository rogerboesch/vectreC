#include "../src/bcdscore.c"
void run(void){ score_bcd[0]=0x12; score_bcd[1]=0x34; score_bcd[2]=0x99;
#ifndef NOKERNEL
 bcd_add(0x77);
#endif
}
