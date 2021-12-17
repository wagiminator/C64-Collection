#!/bin/bash
#
# build.sh, avr-gcc build and installation script
#
# run this script with "sh ./install_avr_gcc.sh"
#
# The default installation target is /usr/local/avr. If you don't
# have the necessary rights to create files there you may change 
# the installation target to e.g. $HOME/avr in your home directory
#

# select where to install the software
export PREFIX=/usr/local/avr
#export PREFIX=$HOME/avr

# tools need each other and must therefore be in path
export PATH=$PATH:$PREFIX/bin

# specify here where to find/install the source archives
ARCHIVES=./archives

# tools required for build process
REQUIRED="wget bison flex gcc g++"

# updated for latest versions on 25/05/07
# versions of tools to be built
BINUTILS=binutils-2.17
# no patches yet for 4.2.0
GCC=gcc-4.1.2
GDB=gdb-6.6
AVRLIBC=avr-libc-1.4.6
AVRDUDE=avrdude-5.4

echo "---------------------------"
echo "- Installation of avr-gcc -"
echo "---------------------------"
echo
echo "Tools to build:"
echo "- ${BINUTILS}"
echo "- ${GCC}"
echo "- ${GDB}"
echo "- ${AVRLIBC}"
echo "- ${AVRDUDE}"
echo
echo "Installation into target directory:"
echo "${PREFIX}"
echo

# Search for required tools
echo "Checking for required tools ..."
for i in ${REQUIRED} ; do 
    echo -n "Checking for $i ...";
    TOOL=`which $i 2>/dev/null`;
    if [ "${TOOL}" == "" ]; then
	echo " not found, please install it!";
        exit 1;
    else
	echo " ${TOOL}, ok"; 
    fi;
done;
echo

# Check if target is already there
if [ -x $PREFIX ]; then 
    echo "Target $PREFIX already exists! This may be due to";
    echo "a previous incomplete build attempt. Please remove";
    echo "it and restart.";
    exit 1;
fi;

# Check if target can be created
install -d $PREFIX >/dev/null
if [ "$?" != "0" ]; then
    echo "Unable to create target $PREFIX, please check";
    echo "permissions.";
    exit 1;
fi;
rmdir $PREFIX

# Check if there's already a avr-gcc
TOOL=`which avr-gcc 2>/dev/null`;
if [ "${TOOL}" != "" ]; then
    echo "There's already a avr-gcc in ${TOOL},";
    echo "please remove it, as it will conflict";
    echo "with the build process!";
    exit 1;
fi;

# Check if source files are sane
check_md5() {
    if [ ! -d ${ARCHIVES} ]; then
	echo "Archive directory ${ARCHIVES} does not exist, creating it."
	install -d ${ARCHIVES}
    fi;

    if [ ! -f ${ARCHIVES}/$1 ]; then
	echo "Source archive ${ARCHIVES}/$1 not found, trying to download it ...";
	wget -O ${ARCHIVES}/$1 $2
    fi;

    echo -n "Verifying md5 sum of $1 ... "
    MD5=`md5sum ${ARCHIVES}/$1 | awk '{print $1}'`
    if [ "$MD5" != "$3" ]; then
	echo "error, file corrupted!";
	echo "MD5 is: $MD5"
	echo "Expected: $3"
	exit 1;
    else
	echo "ok"
    fi;
}

echo "Checking if source files are sane ..."
check_md5 "${BINUTILS}.tar.bz2" \
    ftp://ftp.gnu.org/pub/gnu/binutils/${BINUTILS}.tar.bz2 \
    "e26e2e06b6e4bf3acf1dc8688a94c0d1"
check_md5 "${GCC}.tar.bz2" \
    ftp://ftp.gnu.org/pub/gnu/gcc/${GCC}/${GCC}.tar.bz2 \
    "a4a3eb15c96030906d8494959eeda23c"
check_md5 "${GDB}.tar.bz2" \
    ftp://ftp.gnu.org/pub/gnu/gdb/${GDB}.tar.bz2 \
    "a4df41d28dd514d64e8ccbfe125fd9a6"
check_md5 "${AVRLIBC}.tar.bz2" \
    http://savannah.nongnu.org/download/avr-libc/${AVRLIBC}.tar.bz2 \
    "504b5e3beefc0d500ff94ea88391f71d"
check_md5 "${AVRDUDE}.tar.gz" \
    http://download.savannah.gnu.org/releases/avrdude/${AVRDUDE}.tar.gz \
    "92a58374016bd43c0997a970c895799c"
echo

echo "Build environment seems fine!"
echo

echo "After successful installation add $PREFIX/bin to"
echo "your PATH by e.g. adding"
echo "   export PATH=\$PATH:$PREFIX/bin"
echo "to your $HOME/.bashrc file."
echo

echo "Press return to continue, CTRL-C to abort ..."
read

echo Building binutils ...
tar xvfj ${ARCHIVES}/${BINUTILS}.tar.bz2       
cd ${BINUTILS}
patch -p0 <../binutils-patches/patch-aa
patch -p0 <../binutils-patches/patch-atmega256x
patch -p0 <../binutils-patches/patch-coff-avr
patch -p0 <../binutils-patches/patch-newdevices
mkdir obj-avr
cd obj-avr
../configure  --prefix=$PREFIX --target=avr --disable-nls --enable-install-libbfd
make
make install
cd ../..
rm -rf ${BINUTILS}

echo Building GCC ...
tar xvfj ${ARCHIVES}/${GCC}.tar.bz2
cd ${GCC}
patch -p0 <../gcc-patches/patch-0b-constants
patch -p0 <../gcc-patches/patch-attribute_alias
patch -p0 <../gcc-patches/patch-bug25672
patch -p0 <../gcc-patches/patch-dwarf
patch -p0 <../gcc-patches/patch-libiberty-Makefile.in
patch -p0 <../gcc-patches/patch-newdevices
patch -p0 <../gcc-patches/patch-zz-atmega256x
mkdir obj-avr
cd obj-avr
../configure --prefix=$PREFIX --target=avr --enable-languages=c,c++ --disable-nls
make
make install
cd ../..
rm -rf ${GCC}

echo Building AVR-Libc ...
tar xvfj ${ARCHIVES}/${AVRLIBC}.tar.bz2
cd ${AVRLIBC}
mkdir obj-avr
cd obj-avr
PATH=$PATH:$PREFIX/bin
../configure --prefix=$PREFIX --build=`./config.guess` --host=avr
make
make install
cd ../..
rm -rf ${AVRLIBC}

echo Building GDB ...
tar xvfj ${ARCHIVES}/${GDB}.tar.bz2
cd ${GDB}
mkdir obj-avr
cd obj-avr
../configure --prefix=$PREFIX --target=avr
make
make install
cd ../..
rm -rf ${GDB}

echo Building avrdude ...
tar xvfz ${ARCHIVES}/${AVRDUDE}.tar.gz
cd ${AVRDUDE}
mkdir obj-avr
cd obj-avr
../configure --prefix=$PREFIX
make
make install
cd ../..
rm -rf ${AVRDUDE}
