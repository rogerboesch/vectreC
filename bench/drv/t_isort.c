#include "../src/isort.c"
void run(void){ u8 i; for(i=0;i<N;i++) keys[i]=(s8)(N-i);
#ifndef NOKERNEL
 isort();
#endif
}
