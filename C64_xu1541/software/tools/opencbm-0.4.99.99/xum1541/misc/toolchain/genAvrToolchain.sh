#!/bin/bash
#
# genAvrToolchain.sh, avr-gcc build and installation script,
#                based on a similar script from Till Harbaum
#
# run this script with "bash ./genAvrToolchain.sh"
#
# The default installation target is /usr/local. If you
# don't have the necessary rights to create files there you
# may change the installation target to e.g. $HOME/local in
# your home directory.
#

# select where to install the software
export PREFIX=/usr/local

# please define the base directory for LibUSB's include and
# lib directory, this is especially needed for native
# installations of LibUSB for Windows
# export LIBUSB=/cygdrive/c/Programme/LibUSB-Win32/
export LIBUSB=

# when package specific installation paths should be
# used, the prefixed binary directory must also be
# set in PATH, since tools need to reference each other
#export PREFIX=/usr/local/avr
#export PREFIX=$HOME/avr
#export PREFIX=$HOME/local

#export PATH=$PATH:$PREFIX/bin

# specify here where to find/install the source archives
ARCHIVES=./packages

# specify here where to find/install the WinAVR patches
PATCHES=./patches

# tools required for build process
REQUIRED="wget bison flex gcc g++ gcc-3 sha256sum"
REQLIBS="libgmp libmpc libmpfr"

# which shell to use for configure scripts
# CONFIGSHELL=/bin/ash
# CONFIGSHELL=/usr/local/bin/dash
CONFIGSHELL=/bin/sh

# parallelization of makefile jobs
MULTICORE=11

# do also install the compiled binaries which requires administrative
# rights (comment out to prevent installation)
# DO_INSTALL=yes

if [ ! -f ./sha256sums.txt -o ! -d $ARCHIVES -o ! -d $PATCHES ]
then
	echo please change directory into the path the script resides in
	exit 1
fi

for tool in $REQUIRED
do
	if ! which "$tool"
	then
		echo Prerequsite tool $tool not found in path or not installed.
		FAILED=1
	fi
done
for lib in $REQLIBS
do
	if ! find /usr/lib /usr/local/lib -name "$lib*"
	then
		echo Prerequsite library $lib not found in typical lib directories or not installed.
		FAILED=1
	fi
done
if [ $FAILED ]
then
	echo "Please install the tools and/or Libraries mentioned above."
	exit 1
fi
unset FAILED

# create probably missing directories
if [ ! -d $PATCHES/binutils ]
then
	mkdir $PATCHES/binutils
fi
if [ ! -d $PATCHES/gcc ]
then
	mkdir $PATCHES/gcc
fi
if [ ! -d $PATCHES/avr-libc ]
then
	mkdir $PATCHES/avr-libc
fi


# Only split hole lines, not words
IFS=$'\n'
for line in `grep -v "^#" sha256sums.txt`
do
	case `echo $line | cut -f1,3 | tr -d '\t' | sha256sum -c 2> /dev/null | cut -d: -f2` in
	' OK')
		echo Prerequisite file `echo $line | cut -f3` exists
		;;
	' FAILED open or read')
		if [ `echo $line | cut -f2` = NOT_PUBLICALLY_AVAILABLE ]
		then
			echo
			echo Please also check out `echo $line | cut -f3` from the repository.
			echo
			FAILED=1
		else
			echo Downloading file: `echo $line | cut -f3`
			wget -O `echo $line | cut -d/ -f2- | cut -f1,3 | tr -d '\t'` `echo $line | cut -f2,3 | tr -d '\t'`
		fi
		;;
	' FAILED')
		echo
		echo Checksum mismatch for file: `echo $line | cut -f3`
		echo Please check manually and either correct file sha256sums.txt
		echo or remove the mismatching file above.
		echo
		FAILED=1
		;;
	*)
		echo
		echo Unknown error `echo $line | cut -f1,3 | tr -d '\t' | sha256sum -c | cut -d: -f2` with file `echo $line | cut -f3`
		echo
		FAILED=1
		;;
	esac
done
if [ $FAILED ]
then
	echo
	echo "Please obey error message(s) above."
	exit 1
fi
unset IFS
unset FAILED

# Move old source directories into the delete location (temp dumpster)
LOCALTEMPDIR=`mktemp -d --tmpdir=.`
mv -v binutils-source $LOCALTEMPDIR
mv -v gcc-source      $LOCALTEMPDIR
mv -v avr-libc-source $LOCALTEMPDIR
mv -v avr-gcc         $LOCALTEMPDIR
mv -v avrdude         $LOCALTEMPDIR

if [ -d binutils-source -o -d gcc-source -o -d avr-libc-source -o -d avr-gcc -o -d avrdude-source ]
then
	echo
	echo Cleanup of existing build directories failed, please manually
	echo remove the following directories:
	echo binutils-source
	echo gcc-source
	echo avr-libc-source
	echo avr-gcc
	echo avrdude-source
	exit 1
fi
mkdir -v binutils-source
mkdir -v gcc-source
mkdir -v avr-libc-source
mkdir -v avr-gcc
mkdir -v avrdude-source

# Extract the source archives
echo Extracting: $ARCHIVES/`cut -f3 sha256sums.txt | grep "^binutils-"`
tar -xaf $ARCHIVES/`cut -f3 sha256sums.txt | grep "^binutils-"` --directory=binutils-source --strip-components=1 --checkpoint=1000 --totals
for arg in `cut -f3 sha256sums.txt | grep "^gcc-"`
do
	echo Extracting: $ARCHIVES/$arg
	tar -xaf $ARCHIVES/$arg                                 --directory=gcc-source      --strip-components=1 --checkpoint=1000 --totals
done
echo Extracting: $ARCHIVES/`cut -f3 sha256sums.txt | grep "^avr-libc-"`
tar -xaf $ARCHIVES/`cut -f3 sha256sums.txt | grep "^avr-libc-"` --directory=avr-libc-source --strip-components=1 --checkpoint=1000 --totals
echo Extracting: $ARCHIVES/`cut -f3 sha256sums.txt | grep "^avrdude-"`
tar -xaf $ARCHIVES/`cut -f3 sha256sums.txt | grep "^avrdude"` --directory=avrdude-source --strip-components=1 --checkpoint=1000 --totals

if [ -d $LOCALTEMPDIR ]
then
	cd $LOCALTEMPDIR
	rm -rf * &
	cd ..
fi

# Apply all binutil patches and additional private ones
IFS=$'\n'
for patch in `grep '^ *[^#].*patches/.*[0-9][0-9]*-binutils-.*\.patch$' sha256sums.txt | cut -c67- | sort -n -k+3 | cut -f1,3 | tr -d '\t'`
do
	echo Applying patch: $patch
	patch -b -p 0 -d binutils-source -i "../$patch"
done

# Apply all gcc patches and additional private ones
for patch in `grep '^ *[^#].*patches/.*[0-9][0-9]*-gcc-.*\.patch$' sha256sums.txt | cut -c67- | sort -n -k+3 | cut -f1,3 | tr -d '\t'`
do
	echo Applying patch: $patch
	patch -b -p 0 -d gcc-source -i "../$patch"
done

# Apply all avr-libc patches and additional private ones
for patch in `grep '^ *[^#].*patches/.*[0-9][0-9]*-avr-libc-.*\.patch$' sha256sums.txt | cut -c67- | sort -n -k+3 | cut -f1,3 | tr -d '\t'`
do
	echo Applying patch: $patch
	patch -b -p 0 -d avr-libc-source -i "../$patch"
done

# Apply all avrdude private patches
for patch in `grep '^ *[^#].*patches/.*[0-9][0-9]*-avrdude-.*\.patch$' sha256sums.txt | cut -c67- | sort -n -k+3 | cut -f1,3 | tr -d '\t'`
do
	echo Applying patch: $patch
	patch -b -p 0 -d avrdude-source -i "../$patch"
done
unset IFS

echo
echo "Configuring and making binutils"

cd binutils-source
./configure CONFIG_SHELL=$CONFIGSHELL --prefix=/usr/local --target=avr --program-prefix="avr-"
make -j $MULTICORE
if [ "$DO_INSTALL" = yes ]
then
	make install
fi
if [ $? -ne 0 ]
then
	echo "Installation failed, please run this script as super user"
	exit 1
fi
cd ..

echo
echo "Configuring and making gcc"

mkdir -v avr-gcc
cd avr-gcc
../gcc-source/configure CONFIG_SHELL=$CONFIGSHELL --prefix=/usr/local --target=avr --program-prefix="avr-" --enable-languages=c,c++
make -j $MULTICORE
if [ "$DO_INSTALL" = yes ]
then
	make install
fi
if [ $? -ne 0 ]
then
	echo "Installation failed, please run this script as super user"
	exit 1
fi
cd ..

echo
echo "Configuring and making avr-libc"

cd avr-libc-source
./configure CONFIG_SHELL=$CONFIGSHELL --host=avr
make -j $MULTICORE
if [ "$DO_INSTALL" = yes ]
then
	make install
fi
if [ $? -ne 0 ]
then
	echo "Installation failed, please run this script as super user"
	exit 1
fi
cd ..

echo
echo "Configuring and making avrdude"

cd avrdude-source
./configure CONFIG_SHELL=$CONFIGSHELL CC=gcc-3 CFLAGS="-I $LIBUSB/include/" LDFLAGS="-lusb -L $LIBUSB/lib/gcc/"
# make -j $MULTICORE
# avrdude does not like parallel builds.
make
if [ "$DO_INSTALL" = yes ]
then
	make install
fi
if [ $? -ne 0 ]
then
	echo "Installation failed, please run this script as super user"
	exit 1
fi
cd ..

rmdir $LOCALTEMPDIR

