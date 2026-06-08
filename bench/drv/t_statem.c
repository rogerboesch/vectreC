#include "../src/statem.c"
void run(void){ state=1; timer=5;
#ifndef NOKERNEL
 tick(2);
#endif
}
