##-----------------------------------------------------------------------------
##  Welcome to Streamripper
##-----------------------------------------------------------------------------
project (STREAMRIPPER)

cmake_minimum_required (VERSION 2.8.12)

## Tell CMake to be quiet
cmake_policy (SET CMP0003 NEW)

## Default to release
if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif ()

##-----------------------------------------------------------------------------
##  Include files
##-----------------------------------------------------------------------------
include (CheckFunctionExists)
include (CheckLibraryExists)
include (FindPkgConfig)
include (FindOpenGL)  # Why?

# http://www.cmake.org/pipermail/cmake/2008-December/025886.html
include (InstallRequiredSystemLibraries)


##-----------------------------------------------------------------------------
##  Find libraries
##-----------------------------------------------------------------------------
if (UNIX)
    set (MATH_LIB -lm)
else ()
    set (MATH_LIB)
endif ()

if (UNIX)
  pkg_check_modules (GLIB2 REQUIRED glib-2.0>=2.16)
endif ()

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
find_package (IBERTY)
find_package (MAD)
find_package (OGG)
find_package (Threads)
find_package (VORBIS)

if (WIN32)
  set (MAD_LIBRARIES "${CMAKE_SOURCE_DIR}/win32/libmad-0.15.1b/lib/libmad.lib")
  set (MAD_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/win32/libmad-0.15.1b/include")
endif ()

include (${CMAKE_SOURCE_DIR}/cmake/SR_FindSockets.cmake)

find_library (SEMAPHORE_LIBRARIES NAMES sem rt)

##-----------------------------------------------------------------------------
##  Include directories
##-----------------------------------------------------------------------------
include_directories (BEFORE ${GLIB2_INCLUDE_DIRS})
include_directories (BEFORE ${CMAKE_CURRENT_BINARY_DIR})
include_directories (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/lib)

if (WIN32)
  include_directories (BEFORE 
    "${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.16.6-1/include/glib-2.0")
  include_directories (BEFORE 
    "${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.16.6-1/lib/glib-2.0/include")
  include_directories (BEFORE "${MAD_INCLUDE_DIR}")
  include_directories (BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/win32/iconv")
  include_directories (BEFORE 
    "${CMAKE_CURRENT_SOURCE_DIR}/win32/libogg-1.1.3")
  include_directories (BEFORE 
    "${CMAKE_CURRENT_SOURCE_DIR}/win32/libvorbis-1.1.2")
  include_directories (BEFORE 
    "${CMAKE_CURRENT_SOURCE_DIR}/win32/zlib-1.2.3/include")
endif ()

##-----------------------------------------------------------------------------
##  Configure include file
##-----------------------------------------------------------------------------
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/sr_config.h.cmake 
    ${CMAKE_CURRENT_BINARY_DIR}/sr_config.h)

##-----------------------------------------------------------------------------
##  Copy dlls
##-----------------------------------------------------------------------------
if (WIN32)
  execute_process (COMMAND ${CMAKE_COMMAND} "-E" "copy" 
    "${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.16.6-1/bin/libglib-2.0-0.dll"
    "${CMAKE_CURRENT_BINARY_DIR}")
  execute_process (COMMAND ${CMAKE_COMMAND} "-E" "copy" 
    "${CMAKE_CURRENT_SOURCE_DIR}/win32/mingw/libiconv-2.dll"
    "${CMAKE_CURRENT_BINARY_DIR}")
  execute_process (COMMAND ${CMAKE_COMMAND} "-E" "copy" 
    "${CMAKE_CURRENT_SOURCE_DIR}/win32/mingw/libintl-8.dll"
    "${CMAKE_CURRENT_BINARY_DIR}")
  execute_process (COMMAND ${CMAKE_COMMAND} "-E" "copy" 
    "${CMAKE_CURRENT_SOURCE_DIR}/win32/zlib-1.2.3/zlib1.dll"
    "${CMAKE_CURRENT_BINARY_DIR}")
endif ()

##-----------------------------------------------------------------------------
##  Build subdirectories
##-----------------------------------------------------------------------------
add_subdirectory (lib)

##-----------------------------------------------------------------------------
##  Source files
##-----------------------------------------------------------------------------
set (STREAMRIPPER_SRC
    console/streamripper.c
    )

set (WSTREAMRIPPER_SRC
	winamp_plugin/debug_box.c
	winamp_plugin/debug_box.h
	winamp_plugin/dock.c
	winamp_plugin/dock.h
	winamp_plugin/gen.h
	winamp_plugin/ioapi.c
	winamp_plugin/ioapi.h
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
	winamp_plugin/render_2.c
	winamp_plugin/render_2.h
	winamp_plugin/resource.h
	winamp_plugin/unzip.c
	winamp_plugin/unzip.h
	winamp_plugin/wa_ipc.h
	winamp_plugin/wa_msgids.h
#	winamp_plugin/winamp_dll.c
#	winamp_plugin/winamp_dll.h
	winamp_plugin/winamp_exe.c
	winamp_plugin/winamp_exe.h
	winamp_plugin/wstreamripper.h
	winamp_plugin/Script.rc
	)

##-----------------------------------------------------------------------------
##  Targets
##-----------------------------------------------------------------------------
if (WIN32)
    link_directories (
#	"${CMAKE_CURRENT_SOURCE_DIR}/libmad-0.15.1b/msvc++/Release"
	"${CMAKE_CURRENT_SOURCE_DIR}/win32/iconv"
	"${CMAKE_CURRENT_SOURCE_DIR}/tre-0.7.2/win32/Release"
#	"${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.12.12/lib"
#	"${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.16.3/lib"
#	"c:/gcs6/projects/sripper_1x/glib-gcs/no-unicows"
	"${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.16.6-1/lib"
#	"c:/gcs6/pkg/libunicows/libunicows-1.1.2-msvc6"
#	"c:/gcs6/pkg/libunicows/libunicows-1.1.1-mingw"
	"${CMAKE_CURRENT_SOURCE_DIR}/win32/libogg-1.1.3"
	"${CMAKE_CURRENT_SOURCE_DIR}/win32/libvorbis-1.1.2"
	"${CMAKE_CURRENT_SOURCE_DIR}/win32/zlib-1.2.3/lib"
	)
endif ()

message ("MAD_LIBRARIES = ${MAD_LIBRARIES}")
set (STREAMRIPPER_LIBS ${MAD_LIBRARIES} ${GLIB2_LIBRARIES})

if (WIN32)
    set (STREAMRIPPER_LIBS 
	${STREAMRIPPER_LIBS}
	iconv unicows glib-2.0 unicows ws2_32 ogg vorbis zdll)
endif ()

if (SEMAPHORE_LIBRARIES)
    set (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} ${SEMAPHORE_LIBRARIES})
endif ()

if (IBERTY_FOUND)
    set (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} ${IBERTY_LIBRARIES})
endif ()

if (OGG_FOUND AND VORBIS_FOUND)
    set (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} 
	${OGG_LIBRARIES} ${VORBIS_LIBRARIES})
endif ()

if (CMAKE_THREAD_LIBS_INIT)
    set (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif ()

set (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} ${SR_EXTRA_LIBS} ${MATH_LIB})

##-----------------------------------------------------------------------------
##  Linking
##-----------------------------------------------------------------------------
## chkstk.o may be required for mingw
if (WIN32 AND (MSYS OR MINGW))
    set (STREAMRIPPER_SRC ${STREAMRIPPER_SRC} 
  	${CMAKE_CURRENT_SOURCE_DIR}/win32/chkstk.o)
endif ()

# The old windows dlls are from a previous version of the compiler.
# Might need this in some msvc version.
set (STREAMRIPPER_LDFLAGS "")
if (WIN32)
    set (STREAMRIPPER_LDFLAGS "/SAFESEH:NO")
endif ()

#ADD_LIBRARY (streamripper1 ${STREAMRIPPER_LIB_SRC})
add_executable (streamripper ${STREAMRIPPER_SRC})
target_link_libraries (streamripper 
    streamripper1 ${STREAMRIPPER_LIBS})
if (NOT STREAMRIPPER_LDFLAGS STREQUAL "")
    set_target_properties (streamripper 
	PROPERTIES LINK_FLAGS ${STREAMRIPPER_LDFLAGS})
endif ()

if (WIN32)
    add_executable (wstreamripper WIN32 ${WSTREAMRIPPER_SRC})
    target_link_libraries (wstreamripper 
      streamripper1 ${STREAMRIPPER_LIBS}
      comctl32.lib
      )
  if (NOT STREAMRIPPER_LDFLAGS STREQUAL "")
      set_target_properties (wstreamripper 
	  PROPERTIES LINK_FLAGS ${STREAMRIPPER_LDFLAGS})
  endif ()
endif ()

if (MSVC)
  add_executable (glib_iconv_test lib/glib_iconv_test.c)
  target_link_libraries (glib_iconv_test ${STREAMRIPPER_LIBS})

#  ADD_EXECUTABLE (glib_win98_test lib/glib_win98_test.c)
#  TARGET_LINK_LIBRARIES (glib_win98_test ${STREAMRIPPER_LIBS})
endif ()

##-----------------------------------------------------------------------------
##  Install
##-----------------------------------------------------------------------------
install (TARGETS streamripper DESTINATION bin)
if (WIN32)
    install(TARGETS wstreamripper DESTINATION bin)
endif ()
