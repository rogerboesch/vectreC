#include "../src/rng.c"
void run(void){
#ifndef NOKERNEL
 rng_fill();
#endif
}
