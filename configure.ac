dnl Process this file with autoconf to produce a configure script.
dnl AC_INIT(lib/types.h)

AC_INIT(streamripper,1.61.0b)
AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([lib/config.h])

dnl Checks for programs.
AC_PROG_CC

if test -n "$GCC"; then
        CFLAGS="$CFLAGS -Wall"
fi

AC_PROG_INSTALL

dnl Checks for libraries.
dnl Replace `main' with a function in -lm:
AC_CHECK_LIB(m, main)

dnl Configure options
AC_ARG_ENABLE(static,
    [  --enable-static       	link static against libmad shipped with streamripper],
    enable_static=$enableval, enable_static=no)
AM_CONDITIONAL(STATIC, test $enable_static = yes)

if test "$enable_static" = yes
then
        AC_CONFIG_SUBDIRS(libmad-0.15.1b)
fi

THREADLIBS=no
CPPFLAGS=

AC_CHECK_LIB(pthread,pthread_create,THREADLIBS="-lpthread",)

if test "$THREADLIBS" = no; then
        AC_CHECK_LIB(c_r,pthread_create,THREADLIBS="-pthread",)
fi

if test "$THREADLIBS" = no; then
        AC_CHECK_LIB(dce,pthread_create,THREADLIBS="-ldce",)
fi

if test "$THREADLIBS" = no; then
        AC_CHECK_LIB(pthreads,pthread_create,THREADLIBS="-lpthreads",)
fi

if test "$THREADLIBS" = no; then
        AC_MSG_ERROR(Your system doesn't seem to support posix threads)
        exit
fi

AC_SUBST(THREADLIBS)

if test "$enable_static" = yes
then
	CPPFLAGS="-Ilib -Ilibmad-0.15.1b -D__UNIX__"
	AC_SUBST(CPPFLAGS)
	STATIC_LIBMAD="libmad-0.15.1b/.libs/libmad.a"
	AC_SUBST(STATIC_LIBMAD)
else
	CPPFLAGS="-Ilib -D__UNIX__"
	AC_SUBST(CPPFLAGS)
	STATIC_LIBMAD=""
	AC_SUBST(STATIC_LIBMAD)
        AC_CHECK_LIB(mad, mad_stream_buffer, LIBS="$LIBS $THREADLIBS -lmad", [
                echo "Error! Streamripper needs libmad(-dev) installed in order to compile."
                echo "Try --enable-static to use included libmad."
                exit -1
        ])
fi

dnl Check for semaphore library
AC_CHECK_FUNC(sem_init, , AC_CHECK_LIB(sem, sem_init,))

dnl Check for iconv & related
AM_ICONV
AM_LANGINFO_CODESET

AC_CHECK_LIB(dce,pthread_create,THREADLIBS="-ldce",)

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(mkdir socket strerror strstr)

AC_OUTPUT(Makefile)
