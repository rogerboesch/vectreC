// romdata.c - ROM header pragmas and constant data in ROM.
//
// `const` arrays are placed in the read-only rodata section, which stays in
// cartridge ROM instead of using up the Vectrex's scarce RAM.
//
// Build:
//   cmoc --vectrex -I $env:VECTREC\stdlib -L $env:VECTREC\stdlib -o romdata.bin romdata.c

#include <vectrex/bios.h>

#pragma vx_copyright "2026"
#pragma vx_title_pos -100, -100
#pragma vx_title_size -8, 80
#pragma vx_title "g MY GAME"
#pragma vx_music vx_music_1

const char box[8] = {        // 4 lines of y,x pairs (relative moves)
     50,   0,
      0,  50,
    -50,   0,
      0, -50,
};
const char rom_text[] = "PRESS ANY BUTTON";

int main()
{
    while (1)
    {
        wait_recal();
        intensity_a(0x7f);
        set_scale(0x40);
        moveto_d(-25, -25);
        draw_vl_a(4, (int8_t *)box);
        print_str_c(-100, -60, (char *)rom_text);
    }
    return 0;
}
