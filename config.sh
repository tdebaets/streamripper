aclocal -I m4
automake --add-missing --foreign Makefile
autoconf
autoheader

cd libmad-0.15.1b
aclocal
automake --add-missing --foreign Makefile
autoconf
autoheader
cd ..

rm -f config.cache
## ./configure
./configure --enable-static
