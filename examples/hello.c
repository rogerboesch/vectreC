// hello.c - Minimal Vectrex program: print text every frame.
//
// Build:
//   cmoc --vectrex -I $env:VECTREC\stdlib -L $env:VECTREC\stdlib -o hello.bin hello.c

#include <vectrex/bios.h>

int main()
{
    while (1)
    {
        wait_recal();               // sync to the 50 Hz frame; do this every frame
        intensity_a(0x7f);          // set beam intensity (0x00..0x7f)
        print_str_c(0x10, -0x50, (char *)"HELLO WORLD!");
    }
    return 0;
}
