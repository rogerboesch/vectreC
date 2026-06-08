#include "../src/strupr.c"
char sdst[32]; const char ssrc[]="Hello Vectrex World 123";
void run(void){
#ifndef NOKERNEL
 str_upper(sdst, ssrc);
#endif
}
