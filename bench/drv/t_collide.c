#include "../src/collide.c"
u8 csink;
void run(void){ u8 i; for(i=0;i<NS;i++){ sx[i]=(s8)(i*7-40); sy[i]=(s8)(i*5-30); sw[i]=4; sh[i]=4;}
#ifndef NOKERNEL
 csink=collide_count();
#endif
}
