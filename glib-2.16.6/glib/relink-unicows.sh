gcc -shared .libs/libglib-2.0-0.dll.def \
     -L/usr/local/lib -lunicows \
     .libs/garray.o \
     .libs/gasyncqueue.o .libs/gatomic.o .libs/gbacktrace.o \
     .libs/gbase64.o .libs/gbookmarkfile.o .libs/gcache.o \
     .libs/gchecksum.o .libs/gcompletion.o .libs/gconvert.o \
     .libs/gdataset.o .libs/gdate.o .libs/gdir.o \
     .libs/gerror.o .libs/gfileutils.o .libs/ghash.o \
     .libs/ghook.o .libs/giochannel.o .libs/gkeyfile.o \
     .libs/glist.o .libs/gmain.o .libs/gmappedfile.o \
     .libs/gmarkup.o .libs/gmem.o .libs/gmessages.o \
     .libs/gnode.o .libs/goption.o .libs/gpattern.o \
     .libs/gprimes.o .libs/gqsort.o .libs/gqueue.o \
     .libs/grel.o .libs/grand.o .libs/gregex.o \
     .libs/gscanner.o .libs/gsequence.o .libs/gshell.o \
     .libs/gslice.o .libs/gslist.o .libs/gstdio.o \
     .libs/gstrfuncs.o .libs/gstring.o .libs/gtestutils.o \
     .libs/gthread.o .libs/gthreadpool.o .libs/gtimer.o \
     .libs/gtree.o .libs/guniprop.o .libs/gutf8.o \
     .libs/gunibreak.o .libs/gunicollate.o .libs/gunidecomp.o \
     .libs/gurifuncs.o .libs/gutils.o .libs/gprintf.o \
     .libs/giowin32.o .libs/gspawn-win32.o .libs/gwin32.o \
     -Wl,--whole-archive libcharset/.libs/libcharset.a \
     gnulib/.libs/libgnulib.a pcre/.libs/libpcre.a \
     -Wl,--no-whole-archive \
     -L/usr/local/lib -lws2_32 -lole32 \
     /usr/local/lib/libiconv.dll.a \
     /usr/local/lib/libintl.dll.a -mms-bitfields \
     -Wl,glib-win32-res.o -o .libs/libglib-2.0-0.dll \
     -Wl,--enable-auto-image-base -Xlinker --out-implib \
     -Xlinker .libs/libglib-2.0.dll.a
