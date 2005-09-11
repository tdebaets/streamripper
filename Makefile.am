## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @CPPFLAGS@ @OGG_CFLAGS@ @VORBIS_CFLAGS@

if SUBDIR_LIBMAD
MAYBE_LIBMAD = libmad-0.15.1b
endif

if SUBDIR_TRE
MAYBE_LIBTRE = tre-0.7.0
endif

SUBDIRS = $(MAYBE_LIBMAD) $(MAYBE_LIBTRE)

bin_PROGRAMS = streamripper

EXTRA_PROGRAMS = streamripper ictest

ictest_SOURCES = \
	lib/util.c \
	lib/debug.c \
	lib/ictest.c

##	lib/cbuffer.c \

streamripper_SOURCES = \
	console/streamripper.c \
	lib/cbuf2.c \
	lib/cbuf2.h \
	lib/list.h \
	lib/debug.c \
	lib/filelib.c \
	lib/findsep.c \
	lib/http.c \
	lib/inet.c \
	lib/mpeg.c \
	lib/parse.c \
	lib/relaylib.c \
	lib/rip_manager.c \
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
	lib/parse.h \
	lib/relaylib.h \
	lib/rip_manager.h \
	lib/ripstream.h \
	lib/socklib.h \
	lib/srconfig.h \
	lib/threadlib.h \
	lib/srtypes.h \
	lib/uce_dirent.h \
	lib/util.h

##	lib/ripshout.c \
##	lib/ripshout.h \

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

libmad_dont_need_these = \
	libmad-0.15.1b/msvc++/Makefile.am \
	libmad-0.15.1b/msvc++/Makefile.in \
	libmad-0.15.1b/msvc++/mad.h \
	libmad-0.15.1b/msvc++/config.h \
	libmad-0.15.1b/msvc++/libmad.dsp

tre_manifest = \
	tre-0.7.0/m4/tre_prog_cc_optimizations.m4 \
	tre-0.7.0/m4/ax_check_sign.m4 \
	tre-0.7.0/m4/Makefile.am \
	tre-0.7.0/m4/Makefile.in \
	tre-0.7.0/m4/vl_prog_cc_warnings.m4 \
	tre-0.7.0/m4/ac_libtool_tags.m4 \
	tre-0.7.0/m4/ax_decl_wchar_max.m4 \
	tre-0.7.0/m4/ax_check_funcs_comp.m4 \
	tre-0.7.0/po/Makevars \
	tre-0.7.0/po/fi.po \
	tre-0.7.0/po/LINGUAS \
	tre-0.7.0/po/en@boldquot.header \
	tre-0.7.0/po/en@quot.header \
	tre-0.7.0/po/Rules-quot \
	tre-0.7.0/po/remove-potcdate.sin \
	tre-0.7.0/po/boldquot.sed \
	tre-0.7.0/po/POTFILES.in \
	tre-0.7.0/po/fi.gmo \
	tre-0.7.0/po/tre.pot \
	tre-0.7.0/po/insert-header.sin \
	tre-0.7.0/po/stamp-po \
	tre-0.7.0/po/quot.sed \
	tre-0.7.0/po/Makefile.in.in \
	tre-0.7.0/doc/tre-api.html \
	tre-0.7.0/doc/tre-syntax.html \
	tre-0.7.0/lib/tre-config.h.in \
	tre-0.7.0/lib/regex.h \
	tre-0.7.0/lib/tre-match-approx.c \
	tre-0.7.0/lib/README \
	tre-0.7.0/lib/tre-match-utils.h \
	tre-0.7.0/lib/xmalloc.c \
	tre-0.7.0/lib/xmalloc.h \
	tre-0.7.0/lib/tre-match-backtrack.c \
	tre-0.7.0/lib/tre-internal.h \
	tre-0.7.0/lib/regerror.c \
	tre-0.7.0/lib/tre-compile.c \
	tre-0.7.0/lib/Makefile.am \
	tre-0.7.0/lib/Makefile.in \
	tre-0.7.0/lib/tre-match-parallel.c \
	tre-0.7.0/lib/gettext.h \
	tre-0.7.0/lib/tre-mem.c \
	tre-0.7.0/lib/tre-mem.h \
	tre-0.7.0/lib/regcomp.c \
	tre-0.7.0/lib/regexec.c \
	tre-0.7.0/src/Makefile.am \
	tre-0.7.0/src/Makefile.in \
	tre-0.7.0/src/agrep.c \
	tre-0.7.0/NEWS \
	tre-0.7.0/TODO \
	tre-0.7.0/LICENSE \
	tre-0.7.0/aclocal.m4 \
	tre-0.7.0/tests/test-str-source.c \
	tre-0.7.0/tests/randtest.c \
	tre-0.7.0/tests/retest.c \
	tre-0.7.0/tests/Makefile.am \
	tre-0.7.0/tests/Makefile.in \
	tre-0.7.0/tests/bench.c \
	tre-0.7.0/tests/test-approx.c \
	tre-0.7.0/utils/compile \
	tre-0.7.0/utils/depcomp \
	tre-0.7.0/utils/build-rpm.sh \
	tre-0.7.0/utils/ltmain.sh \
	tre-0.7.0/utils/config.guess \
	tre-0.7.0/utils/config.rpath \
	tre-0.7.0/utils/install-sh \
	tre-0.7.0/utils/autogen.sh \
	tre-0.7.0/utils/config.sub \
	tre-0.7.0/utils/missing \
	tre-0.7.0/utils/mkinstalldirs \
	tre-0.7.0/utils/Makefile.am \
	tre-0.7.0/utils/Makefile.in \
	tre-0.7.0/win32/tre-config.h \
	tre-0.7.0/win32/retest.dsp \
	tre-0.7.0/win32/config.h \
	tre-0.7.0/win32/tre.def \
	tre-0.7.0/win32/tre.dsp \
	tre-0.7.0/win32/tre.dsw \
	tre-0.7.0/README \
	tre-0.7.0/THANKS \
	tre-0.7.0/configure \
	tre-0.7.0/configure.ac \
	tre-0.7.0/tre.spec.in \
	tre-0.7.0/Makefile.am \
	tre-0.7.0/Makefile.in \
	tre-0.7.0/config.h.in \
	tre-0.7.0/python \
	tre-0.7.0/python/setup.py.in \
	tre-0.7.0/AUTHORS \
	tre-0.7.0/INSTALL \
	tre-0.7.0/tre.pc.in \
	tre-0.7.0/ABOUT-NLS \
	tre-0.7.0/ChangeLog \
	tre-0.7.0/COPYING

man_MANS = \
	streamripper.1

EXTRA_DIST = \
	config.rpath \
	CHANGES \
	readme_xfade.txt \
	parse_rules.txt \
	lib/live365info.c \
	lib/live365info.h \
	lib/riplive365.c \
	lib/riplive365.h \
	$(libmad_manifest) \
	$(tre_manifest) \
	$(man_MANS)

DIST_SUBDIRS = 

streamripper_LDADD = @LIBMAD@ @LIBREGEX@ @THREADLIBS@ \
	@LTLIBICONV@ @LIBCHARSET@ \
	@OGG_LIBS@ @VORBIS_LIBS@

ACLOCAL_AMFLAGS = -I m4
