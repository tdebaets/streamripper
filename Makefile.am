## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @CPPFLAGS@

if STATIC
SUBDIRS = libmad-0.15.1b
endif

bin_PROGRAMS = streamripper
streamripper_SOURCES = \
	console/streamripper.c \
	lib/cbuffer.c \
	lib/debug.c \
	lib/filelib.c \
	lib/findsep.c \
	lib/http.c \
	lib/inet.c \
	lib/mpeg.c \
	lib/relaylib.c \
	lib/rip_manager.c \
	lib/ripshout.c \
	lib/ripstream.c \
	lib/socklib.c \
	lib/threadlib.c \
	lib/util.c \
	lib/cbuffer.h \
	lib/compat.h \
	lib/debug.h \
	lib/filelib.h \
	lib/findsep.h \
	lib/http.h \
	lib/inet.h \
	lib/mpeg.h \
	lib/relaylib.h \
	lib/rip_manager.h \
	lib/ripshout.h \
	lib/ripstream.h \
	lib/socklib.h \
	lib/threadlib.h \
	lib/types.h \
	lib/util.h

libmad_manifest = \
	libmad-0.15.1b/README \
	libmad-0.15.1b/configure.ac \
	libmad-0.15.1b/aclocal.m4 \
	libmad-0.15.1b/mad.h \
	libmad-0.15.1b/Makefile.am \
	libmad-0.15.1b/Makefile.in \
	libmad-0.15.1b/config.h.in \
	libmad-0.15.1b/libmad.list.in \
	libmad-0.15.1b/configure \
	libmad-0.15.1b/COPYING \
	libmad-0.15.1b/INSTALL \
	libmad-0.15.1b/TODO \
	libmad-0.15.1b/config.guess \
	libmad-0.15.1b/config.sub \
	libmad-0.15.1b/depcomp \
	libmad-0.15.1b/install-sh \
	libmad-0.15.1b/ltmain.sh \
	libmad-0.15.1b/missing \
	libmad-0.15.1b/mkinstalldirs \
	libmad-0.15.1b/version.c \
	libmad-0.15.1b/fixed.c \
	libmad-0.15.1b/bit.c \
	libmad-0.15.1b/timer.c \
	libmad-0.15.1b/stream.c \
	libmad-0.15.1b/frame.c \
	libmad-0.15.1b/synth.c \
	libmad-0.15.1b/decoder.c \
	libmad-0.15.1b/layer12.c \
	libmad-0.15.1b/layer3.c \
	libmad-0.15.1b/huffman.c \
	libmad-0.15.1b/version.h \
	libmad-0.15.1b/fixed.h \
	libmad-0.15.1b/bit.h \
	libmad-0.15.1b/timer.h \
	libmad-0.15.1b/stream.h \
	libmad-0.15.1b/frame.h \
	libmad-0.15.1b/synth.h \
	libmad-0.15.1b/decoder.h \
	libmad-0.15.1b/global.h \
	libmad-0.15.1b/layer12.h \
	libmad-0.15.1b/layer3.h \
	libmad-0.15.1b/huffman.h \
	libmad-0.15.1b/D.dat \
	libmad-0.15.1b/imdct_s.dat \
	libmad-0.15.1b/qc_table.dat \
	libmad-0.15.1b/rq_table.dat \
	libmad-0.15.1b/sf_table.dat \
	libmad-0.15.1b/imdct_l_arm.S \
	libmad-0.15.1b/minimad.c \
	libmad-0.15.1b/mad.h.sed \
	libmad-0.15.1b/CHANGES \
	libmad-0.15.1b/COPYRIGHT \
	libmad-0.15.1b/CREDITS \
	libmad-0.15.1b/VERSION

dont_need_these = \
	libmad-0.15.1b/msvc++/Makefile.am \
	libmad-0.15.1b/msvc++/Makefile.in \
	libmad-0.15.1b/msvc++/mad.h \
	libmad-0.15.1b/msvc++/config.h \
	libmad-0.15.1b/msvc++/libmad.dsp

man_MANS = \
	streamripper.1

EXTRA_DIST = \
	config.rpath \
	CHANGES \
	readme_xfade.txt \
	lib/live365info.c \
	lib/live365info.h \
	lib/riplive365.c \
	lib/riplive365.h \
	$(libmad_manifest) \
	$(man_MANS)

DIST_SUBDIRS = 

streamripper_LDADD = @STATIC_LIBMAD@ @THREADLIBS@ @LTLIBICONV@
