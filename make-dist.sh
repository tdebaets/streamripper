

mkdir streamripper-1.60
cp README readme_xfade.txt COPYING CHANGES Makefile.in \
    THANKS TODO config.guess configure.in configure config.sub \
    install-sh ltconfig ltmain.sh \
    streamripper-1.60

mkdir streamripper-1.60/console
cp -R console/*.c console/Makefile.in \
    streamripper-1.60/console

cp -R libmad \
    streamripper-1.60
cd streamripper-1.60
rm -rf libmad/CVS
rm -rf libmad/Debug
rm -rf libmad/Release
rm -rf libmad/config.log 
rm -rf libmad/config.cache
rm -rf libmad/config.status
rm -rf libmad/Makefile
rm -rf libmad/stamp-h
cd ..

mkdir streamripper-1.60/lib
cp -R lib/*.h lib/*.c lib/Makefile.in \
    streamripper-1.60/lib

echo Setting timestamps, please wait
cd streamripper-1.60/libmad
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
tar czvf streamripper-1.60.tar.gz streamripper-1.60
