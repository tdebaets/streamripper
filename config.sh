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

rm -f config.cache

## Included glib doesn't work yet.  Include file glib.h not found.
# ./configure --with-included-libmad --with-included-glib

./configure --with-included-libmad

exit

./configure
./configure --without-included-tre
./configure --without-ogg
./configure --with-included-libmad --with-included-tre --with-included-argv

