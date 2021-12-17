#	
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version
#  2 of the License, or (at your option) any later version.
#
#  Copyright 2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
#

package OpenCBM;

use IO::File;
use IO::Handle;

my $VERSION = '0.4.0';

# _IO constants, from cbm_module.h
my $CBMCTRL_TALK=0xcb00;
my $CBMCTRL_LISTEN=0xcb01;
my $CBMCTRL_OPEN=0xcb04;
my $CBMCTRL_CLOSE=0xcb05;
my $CBMCTRL_GETEOI=0xcb07;
my $CBMCTRL_UNTALK=0xcb02;
my $CBMCTRL_UNLISTEN=0xcb03;
my $CBMCTRL_RESET=0xcb07;

my $CBM_DEV="/dev/cbm";

sub DriverOpen()
{
    sysopen(CBM_FD, $CBM_DEV, O_RDWR) || return undef;
    CBM_FD->autoflush(1);
    \CBM_FD;
}

sub DriverClose()
{
    close CBM_FD;
}

sub Listen($$)
{
    my $dev = $_[0];
    my $sa  = $_[1];
    ioctl(CBM_FD, $CBMCTRL_LISTEN, ($dev<<8) | $sa);
}

sub Talk($$)
{
    my $dev = $_[0];
    my $sa  = $_[1];
    ioctl(CBM_FD, $CBMCTRL_TALK, ($dev<<8) | $sa);
}

sub Open($$)
{
    my $dev = $_[0];
    my $sa  = $_[1];
    ioctl(CBM_FD, $CBMCTRL_OPEN, ($dev<<8) | $sa);
}

sub Close($$)
{
    my $dev = $_[0];
    my $sa  = $_[1];
    ioctl(CBM_FD, $CBMCTRL_CLOSE, ($dev<<8) | $sa);
}

sub GetEOI()
{
    ioctl(CBM_FD, $CBMCTRL_GETEOI, 0);
}

sub Unlisten()
{
    ioctl(CBM_FD, $CBMCTRL_UNLISTEN, 0);
}

sub Untalk()
{
    ioctl(CBM_FD, $CBMCTRL_UNTALK, 0);
}

sub Reset()
{
    ioctl(CBM_FD, $CBMCTRL_RESET, 0);
}

1;
