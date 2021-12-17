#	
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version
#  2 of the License, or (at your option) any later version.
#
#  Copyright 2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
#

package CBMFile;

use Fcntl ':flock';
use OpenCBM;
use strict;

my $NumFiles = 0;

sub TIEHANDLE {
    my $self = {};
    my $class = shift;
    $self->{DEV} = shift;
    $self->{SA}  = shift;

    if ($NumFiles == 0)
    {
        OpenCBM::DriverOpen() or return undef;
        OpenCBM::CBM_FD->autoflush(1);
    }
    $NumFiles++;

    $self->{NAME} = shift;

    if(OpenCBM::Open($self->{DEV}, $self->{SA}) && defined $self->{NAME})
    {
        syswrite OpenCBM::CBM_FD, $self->{NAME};
    }
    OpenCBM::Unlisten();

    return bless $self, $class;
}

sub DESTROY {
    OpenCBM::DriverClose() if(--$NumFiles == 0);
}

sub CLOSE {
    my $self = shift;
    OpenCBM::Close( $self->{DEV}, $self->{SA} );
}

sub READ {
    my $self   = shift;
    my $bufref = \$_[0];
    my $len    = $_[1];
    OpenCBM::Talk( $self->{DEV}, $self->{SA} );
    my $l = sysread ( OpenCBM::CBM_FD, $$bufref, $len );
    OpenCBM::Untalk();
    return $l;
}

sub READLINE {
    my $self = shift;
    my $l = undef;
    my $c = "";

    OpenCBM::Talk( $self->{DEV}, $self->{SA} );
    $l .= $c while(defined($c=getc(OpenCBM::CBM_FD)) && (ord($c) != 13));
    $l .= "\n" if defined ($l);
    OpenCBM::Untalk();
    return $l;
}

sub WRITE {
    my $self  = shift;
    my $r;
    OpenCBM::Listen( $self->{DEV}, $self->{SA} );
    $r = syswrite OpenCBM::CBM_FD, @_;
    OpenCBM::Unlisten();
    return $r;
}

sub PRINT {
    my $self  = shift;
    my $r;
    OpenCBM::Listen( $self->{DEV}, $self->{SA} );
    $r = print OpenCBM::CBM_FD @_;

    # flush buffer
    flock OpenCBM::CBM_FD, LOCK_SH;
    flock OpenCBM::CBM_FD, LOCK_UN;

    OpenCBM::Unlisten();
    return $r;
}

sub PRINTF {
    my $self  = shift;
    my $r;
    OpenCBM::Listen( $self->{DEV}, $self->{SA} );
    $r = printf OpenCBM::CBM_FD @_;

    # flush buffer
    flock OpenCBM::CBM_FD, LOCK_SH;
    flock OpenCBM::CBM_FD, LOCK_UN;

    OpenCBM::Unlisten();
    return $r;
}

1;
