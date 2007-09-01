## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @OGG_CFLAGS@ @VORBIS_CFLAGS@

if SUBDIR_LIBMAD
  AM_CPPFLAGS += -I@abs_top_srcdir@/libmad-0.15.1b
endif

if SUBDIR_TRE
  AM_CPPFLAGS += -I@abs_top_srcdir@/tre-0.7.2
endif

noinst_LIBRARIES = libstreamripper.a

libstreamripper_a_SOURCES = \
	cbuf2.c \
	charset.c \
	debug.c \
	external.c \
	filelib.c \
	findsep.c \
	http.c \
	iconvert.c \
	mchar.c \
	mpeg.c \
	parse.c \
	relaylib.c \
	rip_manager.c \
	ripaac.c \
	ripogg.c \
	ripstream.c \
	socklib.c \
	threadlib.c \
	utf8.c \
	cbuf2.h \
	charmaps.h \
	charset.h \
	compat.h \
	debug.h \
	external.h \
	filelib.h \
	findsep.h \
	http.h \
	list.h \
	mchar.h \
	mpeg.h \
	parse.h \
	relaylib.h \
	rip_manager.h \
	ripogg.h \
	ripstream.h \
	socklib.h \
	srconfig.h \
	threadlib.h \
	srtypes.h \
	uce_dirent.h \
	utf8.h
