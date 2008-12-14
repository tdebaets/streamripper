## Process this file with automake to produce Makefile.in

lib_LTLIBRARIES = libtre.la
libtre_la_LDFLAGS = -version-info 6:0:2 $(LDFLAGS)
libtre_la_LIBADD = $(LTLIBINTL)

noinst_HEADERS =		\
	tre-internal.h		\
	tre-match-utils.h	\
	tre-mem.h		\
	xmalloc.h		\
	gettext.h

libtre_la_SOURCES =		\
	tre-mem.c		\
	tre-compile.c		\
	tre-match-parallel.c	\
	tre-match-backtrack.c	\
	regcomp.c		\
	regexec.c		\
	regerror.c

if TRE_APPROX
libtre_la_SOURCES += tre-match-approx.c
endif TRE_APPROX

dist_pkginclude_HEADERS = regex.h
nodist_pkginclude_HEADERS = tre-config.h
