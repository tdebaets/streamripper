

mkdir streamripper-1.60
cp README readme_xfade.txt COPYING CHANGES Makefile.in \
    THANKS TODO config.guess configure.in configure \
    install-sh ltconfig ltmain.sh \
    streamripper-1.60
cp -R console/*.c console/Makefile* \
    streamripper-1.60
cp -R libmad \
    streamripper-1.60
cd streamripper-1.60
rm -rf libmad/CVS
rm -rf libmad/Debug
rm -rf libmad/Release
rm -rf libmad/config.log 
rm -rf libmad/config.cache
rm -rf libmad/config.status
cd ..
cp -R lib/*.h lib/*.c lib/Makefile.in \
    streamripper-1.60

