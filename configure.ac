dnl Process this file with autoconf to produce a configure script.
dnl AC_INIT(lib/types.h)

AC_INIT(streamripper,1.61.2)
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

dnl AC_ARG_ENABLE(static,
dnl     [  --enable-static       	link static against libmad shipped with streamripper],
dnl     enable_static=$enableval, enable_static=no)
dnl AM_CONDITIONAL(STATIC, test $enable_static = yes)

THREADLIBS=no
CPPFLAGS="-Ilib -D__UNIX__"
AC_SUBST(CPPFLAGS)

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


dnl Check for semaphore library
AC_CHECK_FUNC(sem_init, , AC_CHECK_LIB(sem, sem_init,))

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

dnl Check for iconv & related
AM_ICONV
AM_LANGINFO_CODESET
SR_FIND_LOCALE_CHARSET

dnl SR_TEST_DECL

dnl ** GCS Why testing twice??
dnl AC_CHECK_LIB(dce,pthread_create,THREADLIBS="-ldce",)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(mkdir socket strerror strstr)

AC_OUTPUT(Makefile)
