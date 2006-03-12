aclocal -I m4
automake --add-missing --foreign Makefile
autoconf
autoheader

./configure --without-ogg

exit

cd libmad-0.15.1b
aclocal
automake --add-missing --foreign Makefile
autoconf
autoheader
cd ..

cd tre-0.7.2
aclocal -I m4
automake --add-missing --foreign Makefile
autoconf
autoheader
cd ..

rm -f config.cache
## ./configure
./configure --with-included-libmad --with-included-tre --with-included-argv
