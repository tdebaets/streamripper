#! /bin/sh

version=streamripper-1.60.pre1

make clean
if test -d $version; then
  rm -rf $version
fi
mkdir $version
cp README readme_xfade.txt COPYING CHANGES Makefile.in \
    THANKS TODO config.guess configure.in configure config.sub \
    install-sh ltconfig ltmain.sh \
    $version

mkdir $version/console
cp -R console/*.c console/Makefile.in \
    $version/console

cp -R libmad \
    $version
cd $version
rm -rf libmad/CVS
rm -rf libmad/Debug
rm -rf libmad/Release
rm -rf libmad/config.log 
rm -rf libmad/config.cache
rm -rf libmad/config.status
rm -rf libmad/Makefile
rm -rf libmad/stamp-h
cd ..

mkdir $version/lib
cp -R lib/*.h lib/*.c lib/Makefile.in \
    $version/lib

echo Setting timestamps, please wait
cd $version/libmad
sleep 5
touch aclocal.m4
sleep 5
touch configure
sleep 5
touch Makefile.in

cd ..
dos2unix libmad/libtool
dos2unix libmad/mad.h.sed
dos2unix libmad/configure
dos2unix configure
dos2unix config.sub
dos2unix config.guess
dos2unix ltconfig
dos2unix ltmain.sh
dos2unix install-sh
cd ..
tar czvf $version.tar.gz $version
