PROJECT(STREAMRIPPER)

######################################################
##  This isn't really a full cmake, but its enough 
##  for making the msys target.
######################################################

INCLUDE(CheckFunctionExists)
INCLUDE(FindOpenGL)

######################################################
##  MATH LIBRARY
######################################################
IF(UNIX)
   SET(MY_MATH_LIB -lm)
ELSE(UNIX)
   SET(MY_MATH_LIB)
ENDIF(UNIX)

######################################################
##  SOURCE FILES
######################################################
SET(STREAMRIPPER_LIB_SRC
	console/streamripper.c
#	lib/argv.c
	lib/cbuf2.c
	lib/charset.c
	lib/debug.c
	lib/errors.c
	lib/external.c
	lib/filelib.c
	lib/findsep.c
	lib/findsep2.c
	lib/http.c
	lib/iconvert.c
	lib/mchar.c
	lib/parse.c
	lib/prefs.c
	lib/relaylib.c
	lib/ripaac.c
	lib/ripogg.c
	lib/ripstream.c
	lib/ripstream_ogg.c
	lib/rip_manager.c
	lib/socklib.c
	lib/threadlib.c
	lib/utf8.c
	lib/cbuf2.h
	lib/charmaps.h
	lib/charset.h
	lib/compat.h
	lib/confw32.h
	lib/debug.h
	lib/external.h
	lib/filelib.h
	lib/findsep.h
	lib/http.h
	lib/list.h
	lib/mchar.h
	lib/parse.h
	lib/prefs.h
	lib/relaylib.h
	lib/ripogg.h
	lib/ripstream.h
	lib/rip_manager.h
	lib/socklib.h
	lib/srconfig.h
	lib/srtypes.h
	lib/threadlib.h
	lib/uce_dirent.h
	lib/utf8.h
	)

SET(STREAMRIPPER_SRC
	console/streamripper.c
	)

SET(WSTREAMRIPPER_SRC
	winamp_plugin/dock.c
	winamp_plugin/dock.h
	winamp_plugin/gen.h
	winamp_plugin/ipc_pe.h
	winamp_plugin/options.c
	winamp_plugin/options.h
#	winamp_plugin/plugin_dll_main.c
	winamp_plugin/plugin_main.c
	winamp_plugin/plugin_main.h
	winamp_plugin/registry.c
	winamp_plugin/registry.h
	winamp_plugin/render.c
	winamp_plugin/render.h
	winamp_plugin/resource.h
	winamp_plugin/wa_ipc.h
	winamp_plugin/wa_msgids.h
#	winamp_plugin/winamp_dll.c
#	winamp_plugin/winamp_dll.h
	winamp_plugin/winamp_exe.c
	winamp_plugin/winamp_exe.h
	winamp_plugin/wstreamripper.h
#	winamp_plugin/Script.rc
	)


INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.16.6-1/include/glib-2.0)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.16.6-1/lib/glib-2.0/include)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/libmad-0.15.1b/msvc++)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/iconv-win32/static)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/win32/libogg-1.1.3)
INCLUDE_DIRECTORIES(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/win32/libvorbis-1.1.2)


######################################################
##  TARGETS
######################################################
LINK_DIRECTORIES(
	"${CMAKE_CURRENT_SOURCE_DIR}/libmad-0.15.1b/msvc++/Release"
	"${CMAKE_CURRENT_SOURCE_DIR}/iconv-win32/static"
	"${CMAKE_CURRENT_SOURCE_DIR}/tre-0.7.2/win32/Release"
#	"${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.12.12/lib"
#	"${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.16.3/lib"
#	"c:/gcs6/projects/sripper_1x/glib-gcs/no-unicows"
	"${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.16.6-1/lib"
#	"c:/gcs6/pkg/libunicows/libunicows-1.1.2-msvc6"
#	"c:/gcs6/pkg/libunicows/libunicows-1.1.1-mingw"
	"${CMAKE_CURRENT_SOURCE_DIR}/win32/libogg-1.1.3"
	"${CMAKE_CURRENT_SOURCE_DIR}/win32/libvorbis-1.1.2"
	)
#SET(STREAMRIPPER_LIBS libmad iconv tre glib-2.0 ws2_32 ogg vorbis)
#SET(STREAMRIPPER_LIBS libmad iconv glib-2.0 ws2_32 ogg vorbis)
SET(STREAMRIPPER_LIBS libmad iconv unicows glib-2.0 unicows ws2_32 ogg vorbis)

######################################################
##  LINKING
######################################################

## chkstk.o may be required for mingw
IF(MSYS OR MINGW)
  SET(STREAMRIPPER_SRC ${STREAMRIPPER_SRC} 
  		       ${CMAKE_CURRENT_SOURCE_DIR}/win32/chkstk.o)
ENDIF(MSYS OR MINGW)

ADD_LIBRARY(streamripper1 ${STREAMRIPPER_LIB_SRC})
ADD_EXECUTABLE(streamripper ${STREAMRIPPER_SRC})
TARGET_LINK_LIBRARIES(streamripper ${MY_MATH_LIB} 
	   		   streamripper1 ${STREAMRIPPER_LIBS})

ADD_EXECUTABLE(wstreamripper WIN32 ${WSTREAMRIPPER_SRC})
TARGET_LINK_LIBRARIES(wstreamripper ${MY_MATH_LIB} 
	   		   streamripper1 ${STREAMRIPPER_LIBS}
			   comctl32.lib
			   )

IF(MSVC)
ADD_EXECUTABLE(glib_iconv_test lib/glib_iconv_test.c)
TARGET_LINK_LIBRARIES(glib_iconv_test ${STREAMRIPPER_LIBS})

ADD_EXECUTABLE(glib_win98_test lib/glib_win98_test.c)
TARGET_LINK_LIBRARIES(glib_win98_test ${STREAMRIPPER_LIBS})
ENDIF(MSVC)

######################################################
##  INSTALL
######################################################
# INSTALL(TARGETS streamripper DESTINATION bin)
