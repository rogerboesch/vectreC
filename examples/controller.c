// controller.c - Move a small line around with joystick 1.
// Button 1 resets the position to the center.
//
// Build:
//   cmoc --vectrex -I $env:VECTREC\stdlib -L $env:VECTREC\stdlib -o controller.bin controller.c

#include <vectrex/bios.h>

int main()
{
    int8_t y = 0, x = 0;

    controller_enable_1_x();    // joystick 1, x axis
    controller_enable_1_y();    // joystick 1, y axis

    while (1)
    {
        wait_recal();
        intensity_a(0x7f);

        controller_check_joysticks();
        controller_check_buttons();

        if (controller_joystick_1_right()) x += 2;
        if (controller_joystick_1_left())  x -= 2;
        if (controller_joystick_1_up())    y += 2;
        if (controller_joystick_1_down())  y -= 2;

        if (controller_button_1_1_pressed()) { x = 0; y = 0; }

        moveto_d(y, x);
        draw_line_d(20, 0);     // a small vertical line as the "player"
    }
    return 0;
}
