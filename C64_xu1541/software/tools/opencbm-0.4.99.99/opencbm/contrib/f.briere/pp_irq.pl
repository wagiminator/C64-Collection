#!/usr/bin/perl
#
# pp_irq - Figure out if/when a Linux parallel port issues interrupts
#
# Copyright © 2010 Frédéric Brière <fbriere@fbriere.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


use strict;
use warnings;

use Fcntl;
use IO::File;
use Time::HiRes 'sleep';


# How long to sleep while waiting for interrupts
use constant DELAY => 0.2;

# Parallel port constants taken from <linux/parport.h>
use constant {
		PARPORT_CONTROL_STROBE	=> 0x1,
		PARPORT_CONTROL_AUTOFD	=> 0x2,
		PARPORT_CONTROL_INIT	=> 0x4,
		PARPORT_CONTROL_SELECT	=> 0x8,
		
		PARPORT_STATUS_ERROR	=> 0x8,
		PARPORT_STATUS_SELECT	=> 0x10,
		PARPORT_STATUS_PAPEROUT	=> 0x20,
		PARPORT_STATUS_ACK	=> 0x40,
		PARPORT_STATUS_BUSY	=> 0x80,
	};

# ppdev ioctl() constants taken from <linux/ppdev.h>
use constant {
		PPSETMODE	=> 0x40047080,
		PPRSTATUS	=> 0x80017081,
		PPRCONTROL	=> 0x80017083,
		PPWCONTROL	=> 0x40017084,
		PPFCONTROL	=> 0x4002708e,
		PPRDATA		=> 0x80017085,
		PPWDATA		=> 0x40017086,
		PPCLAIM		=> 0x708b,
		PPRELEASE	=> 0x708c,
		PPYIELD		=> 0x708d,
		PPEXCL		=> 0x708f,
		PPDATADIR	=> 0x40047090,
		PPNEGOT		=> 0x40047091,
		PPWCTLONIRQ	=> 0x40017092,
		PPCLRIRQ	=> 0x80047093,
		PPSETPHASE	=> 0x40047094,
		PPGETTIME	=> 0x80087095,
		PPSETTIME	=> 0x40087096,
		PPGETMODES	=> 0x80047097,
		PPGETMODE	=> 0x80047098,
		PPGETPHASE	=> 0x80047099,
		PPGETFLAGS	=> 0x8004709a,
		PPSETFLAGS	=> 0x4004709b,
	};


# Write a single byte over the data lines
sub write_data {
	my ($fh, $byte) = @_;

	$byte = pack "C", $byte;
	$fh->ioctl(PPWDATA, $byte)
		or die "Cannot do ioctl(PPWDATA): $!";
}

# Return and clear the IRQ count
sub irq_count {
	my ($fh) = @_;

	my $buf = pack "i", 0;
	$fh->ioctl(PPCLRIRQ, $buf)
		or die "Cannot do ioctl(PPCLRIRQ): $!";

	return unpack "i", $buf;
}

# Go through a single test run (assuming data is 0x00 at first)
sub run_tests {
	my ($fh) = @_;

	write_data($fh, 0xff);
	sleep DELAY;
	print "Positive edge: ", irq_count($fh), "\n";

	sleep DELAY;
	print "High-level: ", irq_count($fh), "\n";

	write_data($fh, 0x00);
	sleep DELAY;
	print "Negative edge: ", irq_count($fh), "\n";

	sleep DELAY;
	print "Low-level: ", irq_count($fh), "\n";

}

sub main {
	my ($filename, $n) = @_;

	$n ||= 3;

	my $parport = new IO::File;

	$parport->open($filename, O_RDWR)
		or die "Cannot open parport: $!";

	$parport->ioctl(PPEXCL, 0)
		or die "Cannot do ioctl(PPEXCL): $!";
	$parport->ioctl(PPCLAIM, 0)
		or die "Cannot do ioctl(PPCLAIM): $!";

	my $buf = pack "i", 0;
	$parport->ioctl(PPDATADIR, $buf)
		or die "Cannot do ioctl(DATADIR): $!";

	write_data($parport, 0x00);
	irq_count($parport);

	for (1 .. $n) {
		print "Run #$_\n";
		run_tests($parport);
		print "\n";
	}

	$parport->ioctl(PPRELEASE, 0)
		or die "Cannot do ioctl(PPRELEASE): $!";
}

if (@ARGV < 1 || @ARGV > 2) {
	die "Usage: $0 <device> [n_tests]\n";
} else {
	main(@ARGV);
}


__END__

=encoding utf8

=head1 NAME

pp_irq - Figure out if/when a Linux parallel port issues interrupts

=head1 SYNOPSIS

B<pp_irq> I<device> [I<n_tests>]

=head1 DESCRIPTION

I<pp_irq> is a cheap debugging tool that lets you know if/when a Linux
parallel port issues interrupts.  It will cycle the parallel port data lines
high and low, and report on the number of interrupts issued on each step.
This requires connecting the nACK line to one of the data lines, as
illustrated below.

I<device> is the name of a I<ppdev> device (typically F</dev/parport0>);
I<n_tests> is the number of test cycles to run (3 by default).

=head1 DETAILS

=head2 Hardware requirements

I<pp_irq> requires that the parallel port nACK line (#10) be connected to one
of the data lines (#2-9).  This is easily achieved by inserting a metal
paperclip into pins #9 and #10 of the parallel port.  Here's what this would
look like, when looking at the (female) connector from outside the PC case:

		.-----------------------------.
		 \ o o o *~* o o o o o o o o /
		  \ o o o o o o o o o o o o /
		   +-----------------------+

=head2 Software requirements

To use I<pp_irq>, the I<ppdev> module must be loaded, which will provide one
F</dev/parportN> device for each parallel port.

I<pp_irq> requires exclusive usage of the parallel port.  If the port is
already in use, ioctl(PPCLAIM) will fail, and I<syslog> will report the
following:

  parport0: cannot grant exclusive access for device ppdev0

Make sure to stop any daemon using the port (such as CUPS), and unload any
module such as I<lp> or I<gamecon>.

=head1 NOTES

Some evil PCI parallel cards are known to continuously issue interrupts, thus
flooding the Linux kernel and leaving it unable to do anything else while it
tries to service them.  You have been warned.

=head1 AUTHOR

Frédéric Brière <fbriere@fbriere.net>

=cut

