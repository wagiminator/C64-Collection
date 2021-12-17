#!/usr/bin/perl -w
#	
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version
#  2 of the License, or (at your option) any later version.
#
#  Copyright 2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
#

use CBMFile;

use Term::ReadLine;
use strict;

my $drive = $ARGV[0] || 8;

my $term = new Term::ReadLine 'Minimalistic 15*1 disk monitor';
my $prompt = "#$drive > ";

my $block = "";
my $len = 0;
my $track;
my $sector;

my $DNP = "Device not present\n";

tie (*F1, 'CBMFile', $drive, 15);
print F1 'I0:';
print <F1> || $DNP;

while(defined($_ = $term->readline($prompt))) {

    my $cmd = $_;

    last if(m/^q/i);
    next if(m/^ *$/);

    if(m/^@/)
    {
        s/^@//; print F1;
        print <F1> || $DNP;
    }
    elsif(m/^#/)
    {
        s/^# *//; $drive = $_;
        close F1;
        tie (*F1, 'CBMFile', $drive, 15, 'I0:');
        print <F1> || $DNP;
        $prompt = "#$drive > ";
    }
    elsif(m/^\$/)
    {
        tie(*FD, 'CBMFile', $drive, 0, "$_");
        my $status = <F1> || $DNP;
        if($status =~ m/^00/)
        {
            OpenCBM::Talk($drive, 0);
            getc(OpenCBM::CBM_FD);
            getc(OpenCBM::CBM_FD);
            while(getc(OpenCBM::CBM_FD) && getc(OpenCBM::CBM_FD))
            {
                if(defined(my $bl = getc(OpenCBM::CBM_FD)) &&
                   defined(my $bh = getc(OpenCBM::CBM_FD)))
                {
                    print ord($bl) + 256 * ord($bh) . " ";
                    my $c;
                    print $c while(defined($c=getc(OpenCBM::CBM_FD)) &&
                                   (ord($c) != 0));
                    print "\n";
                }
            }
            OpenCBM::Untalk();
            $status = <F1>;
        }
        print $status;
        close FD;
    }
    elsif(m/^[NR]/i)
    {
        if(m/^R/i)
        {
            s/^R[ ]*//i;
            (my $t, my $s) = split(/ /);
            $track = $t if defined ($t);
            $sector = $s if defined ($s);
        }
        elsif($len >= 2)
        {
            $track  = ord(substr($block, 0, 1));
            $sector = ord(substr($block, 1, 1));
        }
        tie (*F2, 'CBMFile', $drive, 2, "#");
        print F1 "U1: 2 0 $track $sector";
        my $status = <F1> || $DNP;
        if($status =~ m/^00/)
        {
            $len = read(F2, $block, 256);
            for my $i (0..$len-1) {
                printf  "%02x", ord(substr($block,$i,1));
                print (($i+1) % 16 ? " " : "\n");
            }
            $prompt = "#$drive [$track $sector] > ";
        }
        close F2;
        print $status;
    }
    elsif(m/^W/i)
    {
        s/^W[ ]*//i;
        (my $t, my $s) = split(/ /);
        $track = $t if defined ($t);
        $sector = $s if defined ($s);
        tie (*F2, 'CBMFile', $drive, 2, "#");
        print F2 $block;
        print F1 "U2: 2 0 $track $sector";
        print <F1> || $DNP;
        close F2;
        $prompt = "#$drive [$track $sector] > ";
    }
    else
    {
        print "Huh?\n";
        next;
    }

    $term->addhistory($cmd) if /\S/;
}

close F1;
