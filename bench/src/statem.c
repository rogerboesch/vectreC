/* statem.c - Game state machine dispatch via switch.
 * The main loop of nearly every game is a switch over the current mode.
 * Exercises: how each compiler lowers switch (jump table vs if/else chain). */
#include "bench.h"

u8 state;
u8 timer;
u16 score;

void tick(u8 ev)
{
    switch (state) {
    case 0:                       /* ATTRACT */
        if (ev == 1) { state = 1; score = 0; timer = 60; }
        break;
    case 1:                       /* PLAY */
        if (timer) timer--;
        else state = 2;
        score = (u16)(score + ev);
        break;
    case 2:                       /* HISCORE */
        if (ev == 2) state = 3;
        break;
    case 3:                       /* GAMEOVER */
        if (ev == 1) state = 0;
        break;
    case 4:                       /* PAUSE */
        if (ev == 3) state = 1;
        break;
    default:
        state = 0;
        break;
    }
}
