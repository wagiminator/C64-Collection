%define name opencbm
%define ver 0.4.0
Summary: OPENCBM/CBM4Linux kernel module, runtime libraries and utilities
Name: %{name}
Version: %{ver}
Release: 1
Group: Applications/System
Copyright: GPL
Source: http://www.lb.shuttle.de/puffin/cbm4linux/%{name}-%{ver}.tar.gz
Buildroot: /var/tmp/%{name}
Url: http://www.lb.shuttle.de/puffin/cbm4linux

Group: Applications/System

%description
The opencbm (cbm4linux) package is a linux kernel module and a few user space
support programs to control and use serial devices as used by most Commodore
(CBM) 8-bit machines, such as disk drives and printers from your trusty C64.  A
fast .d64 transfer program is included.

%package devel
Summary: OPENCBM/CBM4Linux linktime libraries and header files
Group: Development/Libraries
Requires: %{name} = %{ver}

%description devel
Libraries and header files for opencbm/cbm4linux.

%prep
%setup

%build
make

%post
/sbin/ldconfig
/sbin/depmod -a
if [ -e /dev/cbm ]; then
	echo "/dev/cbm exists, leaving owner and permissions untouched"
	ls -l /dev/cbm
else
	echo "creating /dev/cbm"
	mknod -m 660 /dev/cbm c 10 177
	chown root:users /dev/cbm
	ls -l /dev/cbm
fi
install-info --entry="* opencbm: (opencbm).	opencbm users guide" %{_infodir}/%{name}.info.gz %{_infodir}/dir

%preun
if [ "$1" = 0 ]; then
  install-info --delete %{_infodir}/%{name}.info.gz %{_infodir}/dir
fi

%install
rm -rf $RPM_BUILD_ROOT
make install-files \
	PREFIX=$RPM_BUILD_ROOT/%{_prefix} \
	BINDIR=$RPM_BUILD_ROOT/%{_bindir} \
	LIBDIR=$RPM_BUILD_ROOT/%{_libdir} \
	MANDIR=$RPM_BUILD_ROOT/%{_mandir}/man1 \
	INFODIR=$RPM_BUILD_ROOT/%{_infodir} \
	INCDIR=$RPM_BUILD_ROOT/%{_includedir} \
	MODDIR=$RPM_BUILD_ROOT/"`for d in /lib/modules/\`uname -r\`/{extra,misc,kernel/drivers/char}; do test -d $d && echo $d; done | head -n 1`"

%files
%defattr(-,root,root)
%doc README COPYING NEWS CABLE
%doc docs/*
%{_bindir}/*
%{_infodir}/*
%{_libdir}/lib*.so.*
%{_mandir}/man1/*
/lib/modules

%files devel
%defattr(-,root,root)
%{_includedir}/*
%{_libdir}/lib*.so
%{_libdir}/lib*.a

%clean
rm -rf $RPM_BUILD_ROOT
