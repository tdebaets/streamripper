dnl Process this file with autoconf to produce a configure script.
dnl AC_INIT(lib/types.h)

AC_INIT(streamripper,1.61.18)
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

AC_ARG_WITH(included-libmad,
  [  --with-included-libmad use the libmad library included with streamripper],
  force_use_included_libmad=$withval,
  force_use_included_libmad=no)
dnl AC_MSG_RESULT($force_use_included_libmad)

AC_ARG_WITH(included-tre,
  [  --with-included-tre use the tre library included with streamripper],
  force_use_included_tre=$withval,
  force_use_included_tre=no)
dnl AC_MSG_RESULT($force_use_included_tre)

AC_ARG_WITH(included-argv,
  [  --with-included-argv use argv.c included with streamripper],
  force_use_included_argv=$withval,
  force_use_included_argv=no)
dnl AC_MSG_RESULT($force_use_included_argv)

CPPFLAGS="-Ilib -D__UNIX__"
AC_SUBST(CPPFLAGS)

dnl -------- Tests for threading library ----------
THREADLIBS=no
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

dnl -------- Tests for using included libmad ------
LIBMAD=""
AC_SUBST(LIBMAD)
use_included_libmad=yes
if test "$force_use_included_libmad" = no; then
  sr_save_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="-Ilib -D__UNIX__"
  AC_SUBST(CPPFLAGS)
  AC_CHECK_LIB(mad, mad_stream_buffer,
    use_included_libmad=no,
    use_included_libmad=yes)
  CPPFLAGS="$sr_save_CPPFLAGS"
  AC_SUBST(CPPFLAGS)
fi
AM_CONDITIONAL(SUBDIR_LIBMAD, test "$use_included_libmad" = yes)
if test "$use_included_libmad" = yes; then
  LIBMAD="libmad-0.15.1b/.libs/libmad.a"
  AC_SUBST(LIBMAD)
  CPPFLAGS="-Ilibmad-0.15.1b $CPPFLAGS"
  AC_SUBST(CPPFLAGS)
  AC_CONFIG_SUBDIRS(libmad-0.15.1b)
else
  LIBMAD=-lmad
  AC_SUBST(LIBMAD)
fi

dnl -------- Tests for using included tre ------
LIBREGEX=""
AC_SUBST(LIBREGEX)
use_included_tre=yes
if test "$force_use_included_tre" = no; then
  dnl Here is where I verify regex
  dnl Until then, do nothing
  use_included_tre=no
fi
AM_CONDITIONAL(SUBDIR_TRE, test "$use_included_tre" = yes)
if test "$use_included_tre" = yes; then
  LIBREGEX="tre-0.7.0/lib/.libs/libtre.a"
  AC_SUBST(LIBREGEX)
  CPPFLAGS="-Itre-0.7.0/lib $CPPFLAGS"
  AC_SUBST(CPPFLAGS)
  AC_CONFIG_SUBDIRS(tre-0.7.0)
else
  LIBREGEX=""
  AC_SUBST(LIBREGEX)
fi


dnl -------- Tests for using libiberty or argv.c ------
use_included_argv=yes
if test "$force_use_included_argv" = no; then
  AC_CHECK_LIB(iberty, buildargv,
    use_included_argv=no,
    use_included_argv=yes)
fi
AM_CONDITIONAL(USE_INCLUDED_ARGV, test "$use_included_argv" = yes)
if test "$use_included_argv" = yes; then
  LIBIBERTY_LIBS=""
  AC_SUBST(LIBIBERTY_LIBS)
else
  LIBIBERTY_LIBS="-liberty"
  AC_SUBST(LIBIBERTY_LIBS)
fi


dnl -------- Tests for using ogg/vorbis ------
AC_SUBST(VORBIS_CFLAGS)
AC_SUBST(VORBIS_LIBS)
AC_SUBST(OGG_CFLAGS)
AC_SUBST(OGG_LIBS)
dnl -------- (coming soon) ------
sr_have_ogg=no
XIPH_PATH_OGG([
  XIPH_PATH_VORBIS([
    AC_SUBST(OGG_CFLAGS)
    AC_SUBST(OGG_LIBS)
    AC_SUBST(VORBIS_CFLAGS)
    AC_SUBST(VORBIS_LIBS)
    AC_DEFINE(HAVE_OGG_VORBIS,1,[Define to 1 if you have both the ogg and vorbis libraries installed])
    ])
  ])

dnl Check for semaphore library
AC_CHECK_FUNC(sem_init, , AC_CHECK_LIB(sem, sem_init,))
dnl Solaris needs this: -lrt
AC_CHECK_FUNC(sem_init, , AC_CHECK_LIB(rt, sem_init,))

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h)

dnl Check for wchar_t
dnl It seems that it could be in wchar.h, stddef.h, or builtin
AC_CHECK_HEADERS(wchar.h)
AC_CHECK_HEADERS(wctype.h)
AC_CHECK_TYPES([wchar_t],[],[],[
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
#if STDC_HEADERS
#include <stddef.h>
#endif
])

dnl More wide character madness.  FreeBSD doesn't have an iswcntrl().
AC_CHECK_FUNCS(iswcntrl)
AC_FUNC_MBRTOWC

if test "$ac_cv_type_wchar_t" = yes; then
if test "$ac_cv_func_mbrtowc" = yes; then
if test "$ac_cv_func_iswcntrl" = yes; then
  AC_DEFINE(HAVE_WCHAR_SUPPORT,1,[Define to 1 if wide characters is supported])
fi
fi
fi

dnl Check for iconv & related
AM_ICONV
AM_LANGINFO_CODESET
SR_FIND_LOCALE_CHARSET

dnl uint32_t vs. u_int32_t
dnl ISO C 99 requires uint32_t, but u_int32_t is more common(?)
AC_CHECK_HEADERS(stdint.h)
AC_CHECK_HEADERS(inttypes.h)
AC_CHECK_TYPES([uint32_t, u_int32_t])
dnl (maybe these are needed in the future)
dnl AC_CHECK_SIZEOF(short)
dnl AC_CHECK_SIZEOF(int)
dnl AC_CHECK_SIZEOF(long)

dnl Sockets header needed on Solaris
AC_CHECK_HEADERS(sys/sockio.h)

dnl -- GCS: Taken from distcc --
dnl The following test taken from the cvs sources via Samba:
dnl If we can't find connect, try looking in -lsocket, -lnsl, and -linet.
dnl The Irix 5 libc.so has connect and gethostbyname, but Irix 5 also has
dnl libsocket.so which has a bad implementation of gethostbyname (it
dnl only looks in /etc/hosts), so we only look for -lsocket if we need
dnl it.
AC_CHECK_FUNCS(connect)
if test x"$ac_cv_func_connect" = x"no"; then
    case "$LIBS" in
    *-lnsl*) ;;
    *) AC_CHECK_LIB(nsl_s, printf) ;;
    esac
    case "$LIBS" in
    *-lnsl*) ;;
    *) AC_CHECK_LIB(nsl, printf) ;;
    esac
    case "$LIBS" in
    *-lsocket*) ;;
    *) AC_CHECK_LIB(socket, connect) ;;
    esac
    case "$LIBS" in
    *-linet*) ;;
    *) AC_CHECK_LIB(inet, connect) ;;
    esac
    dnl We can't just call AC_CHECK_FUNCS(connect) here, because the value
    dnl has been cached.
    if test x"$ac_cv_lib_socket_connect" = x"yes" || 
       test x"$ac_cv_lib_inet_connect" = x"yes"; then
        dnl ac_cv_func_connect=yes
        dnl don't!  it would cause AC_CHECK_FUNC to succeed next time configure is run
        AC_DEFINE(HAVE_CONNECT,1,[Whether the system has connect()])
    fi
fi
dnl (GCS: I don't think I need these)
dnl AC_CHECK_LIB(resolv, hstrerror, , , [-lnsl -lsocket])
dnl AC_CHECK_LIB(resolv, inet_aton, , , [-lnsl -lsocket])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(mkdir socket strerror strstr)

AC_OUTPUT(Makefile)
