#include "../src/objmove.c"
void run(void){ u8 i; for(i=0;i<NOBJ;i++){ objs[i].x=(s16)(i*200-1500); objs[i].y=(s16)(i*111); objs[i].dx=(s8)(i-8); objs[i].dy=(s8)(7-i);}
#ifndef NOKERNEL
 obj_update();
#endif
}
