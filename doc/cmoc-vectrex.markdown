Using CMOC for Vectrex development
==================================

**By Johan Van den Brande**

`johan@vandenbrande.com`

Introduction
------------

CMOC has support for the Vectrex since version 0.1.17. Since version 0.1.18 most of the BIOS functions are supported. CMOC can be used to write an application for the Vectrex console without the need for an assembler.

Here's a 'Hello World' example:

    #include <vectrex/bios.h>
    
    int main()
    {
      while(1)
      {
        wait_retrace();
        intensity(0x7f);
        print_str_c( 0x10, -0x50, "HELLO WORLD!" );
      }
      return 0;
    }

Which you could compile with:

    cmoc --vectrex hello_world.c

ROM header
----------

A Vectrex cartidge starts with a special header which informs the BIOS a Vectrex cartridge is present. In CMOC we provide the following `pragmas` to customize the header.

### vx\_copyright

Contains a copyright, mostly the year when the application got made.

    // #pragma vx_copyright string
    #pragma vx_copyright "2015"

### vx\_title\_pos

Where to place the title.

    // #pragma vx_title_pos y, x
    #pragma vx_title_pos -100,-100

### vx\_title\_size

How big is the text, height, width. Notice the negative y value.

    // #pragma vx_title_size y, x
    #pragma vx_title_size -8,80

### vx\_title

The title text itself. The 'g' stands for a copyright sign. Only uppercase characters are supported, as lowercase characters contain symbols.

    // #pragma vx_title string
    #pragma vx_title "g NANOFLITE 2015"

### vx\_music

The music played when the console starts up. There are 13 musical pieces available, from vx\_music\_1 to vx\_music\_13

    // #pragma vx_music address
    #pragma vx_music vx_music_2

BIOS
----

The BIOS functions for the Vectrex always have the y coordinate as a first parameter, this is reflected in the C functions. If coordinates are needed, the y-coordinate always comes first.

All the BIOS functions are defined and documented in the file `vectrex/bios.h`.

As the `read_joystick` routine messes up the intensity setting, you have to follow this function by a 'intensity(0x7f)' call or similar.

ROM data
--------

Most Vectrex cartridges only have ROM and so we need to make sure that we do not consume too much RAM. 

We can place our lists of coordinates in ROM by making use of the pragma `const_data`.

Here's an example where we define a rectangle and a string in ROM.

    #pragma const_data start
    char rectangle[8] = {
        -50,  -50,
          0,  100,
       -100,    0,
          0,  -100 
    };
    char rom_text[] = "HELLO VECTREX";
    #pragma const_data end

There can only be one const\_data section.

Emulation
---------

These are the two emulators that I personally use for development:

    * http://www.vectrex.fr/ParaJVE/
    * https://github.com/jhawthorn/vecx

Real hardware
-------------

I use the VecMulti cartridge from Richard Hutchkinson (http://www.vectrex.biz), together with the follwoing command line tools: https://github.com/nanoflite/vecmulti
