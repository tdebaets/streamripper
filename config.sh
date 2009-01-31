aclocal -I m4
automake --add-missing --foreign Makefile
automake --foreign lib/Makefile
autoconf
autoheader

# cd libmad-0.15.1b
# aclocal
# automake --add-missing --foreign Makefile
# autoconf
# autoheader
# cd ..

# cd tre-0.7.2
# aclocal -I m4
# automake --add-missing --foreign Makefile
# autoconf
# autoheader
# cd ..

# cd glib-2.16.6
# aclocal
# libtoolize
# autoheader
# automake --add-missing
# autoconf
# cd ..

# rm -f config.cache
rm -rf autom4te.cache

./configure

exit

./configure
./configure --without-ogg
./configure --with-included-glib
./configure --with-included-libmad
./configure --with-included-libmad --with-included-argv
./configure --with-included-libmad --with-included-glib
