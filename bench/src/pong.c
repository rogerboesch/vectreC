/* pong.c - a complete Vectrex Pong, as a whole-program size benchmark.
 *
 * The game LOGIC (ball physics, paddle AI, collision, scoring, building the
 * draw list) is portable C and identical across compilers.  The Vectrex BIOS
 * routines are declared `extern` so every compiler emits the same call sites
 * (a JSR to the routine); the BIOS bodies themselves live in ROM and are not
 * part of what we measure - exactly as runtime-helper bodies were excluded in
 * the kernel benchmarks. */
#include "bench.h"

/* ------- Vectrex BIOS routines (resolved to fixed ROM addresses at link) --- */
extern void wait_recal(void);
extern void intensity_a(u8 i);
extern void reset0ref(void);
extern void set_scale(u8 s);
extern void moveto_d(s8 y, s8 x);          /* set beam to relative position   */
extern void draw_line_d(s8 y, s8 x);       /* draw relative line              */
extern void draw_vl(const s8 *list);       /* draw a (count,y,x...) line list */
extern void print_str_d(s8 y, s8 x, const char *s);
extern s8   joy1_y(void);                  /* analog joystick 1, Y axis       */
extern u8   button_pressed(u8 which);

/* ------------------------------- constants -------------------------------- */
#define TOP      100
#define BOT     (-100)
#define LWALL   (-110)
#define RWALL    110
#define PADDLE_H  24
#define PADDLE_HH 12
#define BALL_HALF  3
#define WIN_SCORE  9

/* --------------------------------- state ---------------------------------- */
s16 ball_x, ball_y;        /* ball centre (Q4: 16x sub-pixel)  */
s8  ball_dx, ball_dy;      /* velocity                          */
s8  p1_y, p2_y;            /* paddle centres                    */
u8  score1, score2;
u16 rng = 0x1234;

/* small box outline used to draw the ball: (count, then y,x pairs) */
const s8 ball_shape[9] = { 4,  6,0,  0,6,  -6,0,  0,-6 };

static u8 next_rand(void)
{
    rng ^= (u16)(rng << 7);
    rng ^= (u16)(rng >> 9);
    return (u8)rng;
}

void serve(s8 dir)
{
    ball_x = 0;
    ball_y = 0;
    ball_dx = dir;
    ball_dy = (s8)((next_rand() & 3) - 1);
    if (ball_dy == 0) ball_dy = 1;
}

void reset_game(void)
{
    score1 = 0;
    score2 = 0;
    p1_y = 0;
    p2_y = 0;
    serve(2);
}

/* paddle AI: track the ball, clamped to a max speed and to the play field */
static s8 track(s8 y)
{
    s8 by = (s8)(ball_y >> 4);
    if (by > (s8)(y + 2))      y = (s8)(y + 3);
    else if (by < (s8)(y - 2)) y = (s8)(y - 3);
    if (y >  (s8)(TOP - PADDLE_HH)) y = (s8)(TOP - PADDLE_HH);
    if (y <  (s8)(BOT + PADDLE_HH)) y = (s8)(BOT + PADDLE_HH);
    return y;
}

/* does the ball (at pixel bx,by) overlap a paddle centred at (px,py)? */
static u8 hit_paddle(s8 bx, s8 by, s8 px, s8 py)
{
    s8 dx = (s8)(bx - px);
    if (dx < 0) dx = (s8)-dx;
    s8 dy = (s8)(by - py);
    if (dy < 0) dy = (s8)-dy;
    return (u8)(dx <= (BALL_HALF + 2) && dy <= (PADDLE_HH + BALL_HALF));
}

/* advance the ball one tick; returns 1 if a point was scored */
u8 update_ball(void)
{
    ball_x = (s16)(ball_x + ball_dx);
    ball_y = (s16)(ball_y + ball_dy);

    s8 bx = (s8)(ball_x >> 4);
    s8 by = (s8)(ball_y >> 4);

    if (by >= TOP) { ball_dy = (s8)-ball_dy; ball_y = (s16)(TOP << 4); }
    if (by <= BOT) { ball_dy = (s8)-ball_dy; ball_y = (s16)(BOT << 4); }

    if (hit_paddle(bx, by, (s8)(LWALL + 6), p1_y) && ball_dx < 0) {
        ball_dx = (s8)-ball_dx;
        ball_dy = (s8)(ball_dy + ((by - p1_y) >> 3));
    }
    if (hit_paddle(bx, by, (s8)(RWALL - 6), p2_y) && ball_dx > 0) {
        ball_dx = (s8)-ball_dx;
        ball_dy = (s8)(ball_dy + ((by - p2_y) >> 3));
    }

    if (bx <= LWALL) { score2++; serve(2);  return 1; }
    if (bx >= RWALL) { score1++; serve(-2); return 1; }
    return 0;
}

/* ------------------------------- rendering -------------------------------- */
static void draw_paddle(s8 x, s8 y)
{
    moveto_d((s8)(y - PADDLE_HH), x);
    draw_line_d(PADDLE_H, 0);
}

static void draw_ball(void)
{
    moveto_d((s8)(ball_y >> 4), (s8)(ball_x >> 4));
    draw_vl(ball_shape);
}

static void draw_court(void)
{
    s8 y;
    for (y = BOT; y < TOP; y = (s8)(y + 16)) {
        moveto_d(y, 0);
        draw_line_d(8, 0);
    }
}

static void draw_scores(void)
{
    char buf[2];
    buf[1] = 0;
    buf[0] = (char)('0' + (score1 % 10));
    print_str_d(110, -40, buf);
    buf[0] = (char)('0' + (score2 % 10));
    print_str_d(110, 30, buf);
}

void render(void)
{
    reset0ref();
    intensity_a(0x60);
    draw_court();
    draw_paddle((s8)(LWALL + 6), p1_y);
    draw_paddle((s8)(RWALL - 6), p2_y);
    draw_ball();
    draw_scores();
}

/* ----------------------------- main game loop ----------------------------- */
int main(void)
{
    reset_game();
    for (;;) {
        wait_recal();
        set_scale(0x7f);

        /* input: player drives the left paddle, AI drives the right */
        s8 j = joy1_y();
        if (j > 16  && p1_y < (s8)(TOP - PADDLE_HH)) p1_y = (s8)(p1_y + 3);
        if (j < -16 && p1_y > (s8)(BOT + PADDLE_HH)) p1_y = (s8)(p1_y - 3);
        p2_y = track(p2_y);

        if (update_ball()) {
            if (score1 >= WIN_SCORE || score2 >= WIN_SCORE)
                reset_game();
        }
        if (button_pressed(0)) reset_game();

        render();
    }
    return 0;
}
