# Copyright (C) 2007-2011 LuaDist.
# Created by Peter Drahoš, Peter Kapec
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
# Please note that the package source code is licensed under its own license.
# Changed by Vinicius Jarina (viniciusjarina@gmail.com)

PROJECT ( lua C )
CMAKE_MINIMUM_REQUIRED ( VERSION 2.8.7 )

IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET (LIB_SUFFIX "64" CACHE STRING "Suffix of the directory name, e.g. 64 for lib64")
ELSE (CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET (LIB_SUFFIX "" CACHE STRING "Suffix of the directory name, e.g. 64 for lib64")
ENDIF (CMAKE_SIZEOF_VOID_P EQUAL 8)

SET (CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

# $ORIGIN support in rpath.
IF (UNIX AND NOT APPLE)
    SET (USE_RPATH_ORIGIN TRUE)
    SET (CMAKE_INSTALL_RPATH "\$ORIGIN/..")
ENDIF ()

IF (APPLE)
  SET(SKIP_BUILD_RPATH, TRUE)
  SET(CMAKE_MACOSX_RPATH 1)
  ADD_DEFINITIONS (-DLUA_USE_MACOSX)
  LIST ( APPEND LIBS readline )
ENDIF ()

SET(CMAKE_DEBUG_POSTFIX "d")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" CACHE STRING "Library build output directory")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
SET(EXECUTABLE_OUTPUT_PATH         "${CMAKE_BINARY_DIR}")

SET (LIB_LUA_VER "54" CACHE STRING "Suffix of version (for library name) e.g lua54")

#
# Include dir COPY
# If the source dir and the binary dir are not equals,
# copy the content of libzond/cinlude to $/include
#
IF(NOT ${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
    SET(TEMP_INCLUDE_LUA_DIR_FROM "${PROJECT_SOURCE_DIR}/include")
    SET(TEMP_INCLUDE_LUA_DIR_TO   "${PROJECT_BINARY_DIR}")
    MESSAGE(STATUS "Copying include dirs from  ${TEMP_INCLUDE_LUA_DIR_FROM} to ${TEMP_INCLUDE_LUA_DIR_TO}")
    FILE ( COPY ${TEMP_INCLUDE_LUA_DIR_FROM} DESTINATION ${TEMP_INCLUDE_LUA_DIR_TO})
ENDIF()

IF ( WIN32 AND NOT CYGWIN )
  # Windows systems
  OPTION ( LUA_WIN "Windows specific build." ON )
  OPTION ( LUA_BUILD_AS_DLL "Build  Lua library as Dll." ON )

ELSE ()
  # Posix systems (incl. Cygwin)
  OPTION ( LUA_USE_POSIX "Use POSIX functionality." ON )
  OPTION ( LUA_USE_DLOPEN "Use dynamic linker to load modules." ON )
  OPTION ( LUA_INCLUDE_TEST "Include ltests.h")
ENDIF ()

IF ( LUA_USE_POSIX )
  # On POSIX Lua links to standard math library "m"
  LIST ( APPEND LIBS m )
  ADD_DEFINITIONS (-DLUA_USE_POSIX)
ENDIF ()

IF ( LUA_USE_DLOPEN )
  # Link to dynamic linker library "dl"
  LIST ( APPEND LIBS dl )
ENDIF ()

# Compiler options taken from Lua: lua/blob/master/makefile
ADD_COMPILE_OPTIONS (-fPIC
    -fmax-errors=5
    -Wall
    -Wextra
    -Wshadow
    -Wsign-compare
    -Wundef
    -Wwrite-strings
    -Wredundant-decls
    -Wdisabled-optimization
    -Wdouble-promotion
    -Wlogical-op
    -Wno-aggressive-loop-optimizations
    # the next warnings might be useful sometimes, but usually they generate too much noise
    # -Werror
    -Wconversion
    -Wno-sign-conversion
    -Wstrict-overflow=2
    -Wformat=2
    -Wcast-qual
)

# The next warnings are neither valid nor needed for C++
SET (CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "\
    -Wdeclaration-after-statement \
    -Wmissing-prototypes \
    -Wnested-externs \
    -Wstrict-prototypes \
    -Wc++-compat \
    -Wold-style-definition"
)

if ( LUA_INCLUDE_TEST )
    ADD_DEFINITIONS (-DLUA_USER_H="ltests.h" -O0 -g)
ENDIF ()

IF ( LUA_WIN )
  # Use static linked runtime for DLL to avoid issues with people without the vcredist installed
  IF (MSVC)
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    SET(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_DEBUG} /MT")
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    SET(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    SET(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_DEBUG} /MT")
  ENDIF()
  # Add extra rc files to the windows build
  IF ( MSVC OR MINGW )
    SET ( LUA_DLL_RC src/lua_dll.rc )
    SET ( LUA_RC src/lua.rc )
    SET ( LUAC_RC src/luac.rc )
  ENDIF ()
ENDIF ()


# Sources and headers
INCLUDE_DIRECTORIES ( . )
SET (   SRC_LIB
        lapi.c
        lauxlib.c
        lbaselib.c
        lcode.c
        lcorolib.c
        lctype.c
        ldblib.c
        ldebug.c
        ldo.c
        ldump.c
        lfunc.c
        lgc.c
        linit.c
        liolib.c
        llex.c
        lmathlib.c
        lmem.c
        loadlib.c
        lobject.c
        lopcodes.c
        loslib.c
        lparser.c
        lstate.c
        lstring.c
        lstrlib.c
        ltable.c
        ltablib.c
        ltm.c
        lundump.c
        lutf8lib.c
        lvm.c
        lzio.c
        lgrit.c
        lgrit_math.c
)

SET ( SRC_LUA  lua.c )
SET ( SRC_LUAC luac.c )
if ( LUA_INCLUDE_TEST )
    SET (SRC_LIB ${SRC_LIB} ltests.c)
ENDIF ()


## BUILD
# Create dynamic library (dll, so)

ADD_LIBRARY ( liblua SHARED ${SRC_LIB} ${LUA_DLL_RC})

TARGET_LINK_LIBRARIES ( liblua ${LIBS} )
SET_TARGET_PROPERTIES ( liblua PROPERTIES OUTPUT_NAME lua${LIB_LUA_VER} CLEAN_DIRECT_OUTPUT 1 )

if ( LUA_WIN )
    SET_TARGET_PROPERTIES ( liblua PROPERTIES COMPILE_FLAGS -DLUA_BUILD_AS_DLL)
endif ( LUA_WIN )

SET_TARGET_PROPERTIES(  liblua PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES(  liblua PROPERTIES LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES(  liblua PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${PROJECT_BINARY_DIR} )

SET_TARGET_PROPERTIES(  liblua PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES(  liblua PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES(  liblua PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR} )

SET_TARGET_PROPERTIES(  liblua PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES(  liblua PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES(  liblua PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR} )

SET_TARGET_PROPERTIES(  liblua PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES(  liblua PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES(  liblua PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR} )


# Create static library, this is needed to compile luac in the 5.1.x, 5.2 Lua series

ADD_LIBRARY ( liblua_static STATIC ${SRC_LIB} )
TARGET_LINK_LIBRARIES ( liblua_static ${LIBS} )

SET_TARGET_PROPERTIES ( liblua_static PROPERTIES OUTPUT_NAME lua${LIB_LUA_VER}_static CLEAN_DIRECT_OUTPUT 1 )

SET_TARGET_PROPERTIES(  liblua_static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES(  liblua_static PROPERTIES LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${PROJECT_BINARY_DIR} )

SET_TARGET_PROPERTIES(  liblua_static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES(  liblua_static PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${PROJECT_BINARY_DIR} )

SET_TARGET_PROPERTIES(  liblua_static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES(  liblua_static PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR} )

SET_TARGET_PROPERTIES(  liblua_static PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES(  liblua_static PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR} )

ADD_EXECUTABLE ( lua ${SRC_LUA} ${LUA_RC})
TARGET_LINK_LIBRARIES ( lua liblua ${LIBS})

SET_TARGET_PROPERTIES ( lua PROPERTIES DEBUG_POSTFIX d)

SET_TARGET_PROPERTIES ( lua PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG          ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES ( lua PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES ( lua PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE        ${PROJECT_BINARY_DIR} )
SET_TARGET_PROPERTIES ( lua PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     ${PROJECT_BINARY_DIR} )

if (NOT LUA_INCLUDE_TEST )
    ADD_EXECUTABLE ( luac ${SRC_LUAC} ${LUAC_RC} )
    TARGET_LINK_LIBRARIES ( luac liblua_static )

    SET_TARGET_PROPERTIES ( luac PROPERTIES DEBUG_POSTFIX d)

    SET_TARGET_PROPERTIES ( luac PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG          ${PROJECT_BINARY_DIR} )
    SET_TARGET_PROPERTIES ( luac PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${PROJECT_BINARY_DIR} )
    SET_TARGET_PROPERTIES ( luac PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE        ${PROJECT_BINARY_DIR} )
    SET_TARGET_PROPERTIES ( luac PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     ${PROJECT_BINARY_DIR} )
ENDIF()

ENABLE_TESTING()

SET(TEST_PARAM -e _U=true)

ADD_TEST(NAME test_all
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/testes
        COMMAND all.lua
    )

