#!/usr/bin/perl -w
#
# Changes lower-case ",pcr" to ",Y", as per the convention that
# upper-case ",PCR" is a reference to code or to a read-only global.

use strict;


while (<>)
{
    s/,pcr/,Y/;  # case sensitive
    print;
}
