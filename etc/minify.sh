#!/bin/sh
################################################################################
# minify.sh: A script to generate Lua in a single header. Note, GLM is a C++
# library and the generated header is to be used in C++ environments.
#
# Usage: minify.sh [LUA_DIRECTORY] [HEADER_FILE]
#	LUA_DIRECTORY: Directory of LuaGLM repository (default: ".")
#	HEADER_FILE: Generated header name (default "minilua.hpp")
#
# Makefile Rules:
# 	minilua.hpp:
#		./etc/minify.sh ${PWD} minilua.hpp
#
# 	minilua: minilua.hpp
#		echo "#define LUA_IMPL\n#define MAKE_LUA\n#define LUA_INCLUDE_LIBGLM\n#include \"minilua.hpp\"" | \
#		$(CPP) $(CFLAGS) $(CPERF_FLAGS) -DLUA_USE_READLINE -o lua -x c++ - -Wl,-E -ldl -lreadline
#
# 	miniluac: minilua.hpp
#		echo "#define LUA_IMPL\n#define MAKE_LUAC\n#include \"minilua.hpp\"" | \
#		$(CPP) $(CFLAGS) $(CPERF_FLAGS) -o luac -x c++ -
#
# Inspired by:
#   [LuaJIT](https://github.com/LuaJIT/LuaJIT) & genminilua.lua.
#   [minilua](https://github.com/edubart/minilua).
################################################################################
LUA_DIR=${1-"."}
OUTFILE=${2-"minilua.hpp"}

rm -f ${OUTFILE}
cat << EOF >> ${OUTFILE}
/*
** Lua core, libraries, and interpreter in a single header. Define 'LUA_IMPL' in
** a single unit to build the implementation.
**
** @OPTIONS:
**  MAKE_LUA - include the Lua command line interpreter (lua.c)
**  MAKE_LUAC - include the Lua command line compiler (luac.c)
**  LUA_INCLUDE_LIBGLM - include GLM binding library; this library is disabled-by-default.
**
** @EXAMPLE: Build the Lua interpreter:
**    #define LUA_IMPL
**    #define MAKE_LUA
**    #include \"${OUTFILE}\"
**
** See Copyright Notice at the end of this file
*/
#undef LUA_C_LINKAGE
#if !defined(__cplusplus)
  #error "LuaGLM must be compiled with cplusplus for GLM integration"
#endif

/* default is to build the library; @NOTE this deviates from onelua.c */
#ifndef MAKE_LIB
  #ifndef MAKE_LUAC
    #ifndef MAKE_LUA
      #define MAKE_LIB
    #endif
  #endif
#endif

/* choose suitable platform-specific features */
/* some of these may need extra libraries such as -ldl -lreadline -lncurses */
#if 0
  #define LUA_USE_LINUX
  #define LUA_USE_MACOSX
  #define LUA_USE_POSIX
  #define LUA_ANSI
#endif

/* Automatic system configuration; @NOTE this deciates from onelua.c */
#if !defined(LUA_USE_WINDOWS) && !defined(LUA_USE_LINUX) && !defined(LUA_USE_MACOSX) && !defined(LUA_USE_POSIX) && !defined(LUA_USE_C89)
  #if defined(_WIN32)
    #define LUA_USE_WINDOWS
  #elif defined(__linux__)
    #define LUA_USE_LINUX
  #elif defined(__APPLE__)
    #define LUA_USE_MACOSX
  #else
    #define LUA_USE_POSIX  /* Assume the system is POSIX-compliant */
    #define LUA_USE_DLOPEN
  #endif
#endif

/* no need to change anything below this line ----------------------------- */
EOF

echo "#if defined(LUA_IMPL)" >> ${OUTFILE}
  cat ${LUA_DIR}/lprefix.h >> ${OUTFILE}
echo "#endif /* LUA_IMPL */" >> ${OUTFILE}

cat << EOF >> ${OUTFILE}
/* #include <assert.h> */
/* #include <ctype.h> */
/* #include <errno.h> */
/* #include <float.h> */
/* #include <limits.h> */
/* #include <locale.h> */
/* #include <math.h> */
/* #include <setjmp.h> */
/* #include <signal.h> */
/* #include <stdarg.h> */
/* #include <stddef.h> */
/* #include <stdio.h> */
/* #include <stdlib.h> */
/* #include <string.h> */
/* #include <time.h> */

/* setup for luaconf.h */
#define LUA_CORE
#define LUA_LIB
/* #define LUA_INCLUDE_LIBGLM */ /* Disabled by default; include in build unit */
#define ltable_c
#define lvm_c
EOF
cat ${LUA_DIR}/luaconf.h >> ${OUTFILE}

cat << EOF >> ${OUTFILE}
/* do not export internal symbols */
#undef LUAI_FUNC
#undef LUAI_DDEC
#undef LUAI_DDEF
#define LUAI_FUNC static
#define LUAI_DDEC(def) /* empty */
#define LUAI_DDEF static
EOF

# Headers exported by lua.hpp
cat ${LUA_DIR}/lua.h >> ${OUTFILE}
cat ${LUA_DIR}/lualib.h >> ${OUTFILE}
echo "/* auxiliary library -- used by all */" >> ${OUTFILE}
cat ${LUA_DIR}/lauxlib.h >> ${OUTFILE}

# Internal headers. Only included when compiling the interpreter.
echo "#if defined(LUA_IMPL)" >> ${OUTFILE}
  echo "/* core -- used by all */" >> ${OUTFILE}
  cat ${LUA_DIR}/llimits.h >> ${OUTFILE}
  cat ${LUA_DIR}/lobject.h >> ${OUTFILE}
  cat ${LUA_DIR}/lmem.h >> ${OUTFILE}
  cat ${LUA_DIR}/ltm.h >> ${OUTFILE}
  cat ${LUA_DIR}/lstate.h >> ${OUTFILE}
  cat ${LUA_DIR}/lzio.h >> ${OUTFILE}
  cat ${LUA_DIR}/lopcodes.h >> ${OUTFILE}
  cat ${LUA_DIR}/lopnames.h >> ${OUTFILE}
  cat ${LUA_DIR}/ldebug.h >> ${OUTFILE}
  cat ${LUA_DIR}/ldo.h >> ${OUTFILE}
  cat ${LUA_DIR}/lgc.h >> ${OUTFILE}
  cat ${LUA_DIR}/lfunc.h >> ${OUTFILE}
  cat ${LUA_DIR}/lstring.h >> ${OUTFILE}
  cat ${LUA_DIR}/lundump.h >> ${OUTFILE}
  cat ${LUA_DIR}/lapi.h >> ${OUTFILE}
  cat ${LUA_DIR}/llex.h >> ${OUTFILE}
  cat ${LUA_DIR}/ltable.h >> ${OUTFILE}
  cat ${LUA_DIR}/lparser.h >> ${OUTFILE}
  cat ${LUA_DIR}/lcode.h >> ${OUTFILE}
  cat ${LUA_DIR}/lvm.h >> ${OUTFILE}
  cat ${LUA_DIR}/lctype.h >> ${OUTFILE}

  echo "/* luaglm */" >> ${OUTFILE}
  cat ${LUA_DIR}/lglm_core.h >> ${OUTFILE}
  cat ${LUA_DIR}/lgrit_lib.h >> ${OUTFILE} # Designed to be used externally
  cat ${LUA_DIR}/lglm.hpp >> ${OUTFILE} # Also designed to be used externally
  cat ${LUA_DIR}/lglm_string.hpp >> ${OUTFILE}

  echo "/* core -- used by all */" >> ${OUTFILE}
  cat ${LUA_DIR}/lzio.c >> ${OUTFILE}
  cat ${LUA_DIR}/lctype.c >> ${OUTFILE}
  cat ${LUA_DIR}/lopcodes.c >> ${OUTFILE}
  cat ${LUA_DIR}/lmem.c >> ${OUTFILE}
  cat ${LUA_DIR}/lundump.c >> ${OUTFILE}
  cat ${LUA_DIR}/ldump.c >> ${OUTFILE}
  cat ${LUA_DIR}/lstate.c >> ${OUTFILE}
  cat ${LUA_DIR}/lgc.c >> ${OUTFILE}
  cat ${LUA_DIR}/llex.c >> ${OUTFILE}
  cat ${LUA_DIR}/lcode.c >> ${OUTFILE}
  cat ${LUA_DIR}/lparser.c >> ${OUTFILE}
  cat ${LUA_DIR}/ldebug.c >> ${OUTFILE}
  cat ${LUA_DIR}/lfunc.c >> ${OUTFILE}
  cat ${LUA_DIR}/lobject.c >> ${OUTFILE}
  cat ${LUA_DIR}/ltm.c >> ${OUTFILE}
  cat ${LUA_DIR}/lstring.c >> ${OUTFILE}
  cat ${LUA_DIR}/ltable.c >> ${OUTFILE}
  cat ${LUA_DIR}/ldo.c >> ${OUTFILE}
  cat ${LUA_DIR}/lvm.c >> ${OUTFILE}
  cat ${LUA_DIR}/lapi.c >> ${OUTFILE}
  cat ${LUA_DIR}/lglm.cpp >> ${OUTFILE}

  echo "/* auxiliary library -- used by all */" >> ${OUTFILE}
  cat ${LUA_DIR}/lauxlib.c >> ${OUTFILE}

  echo "#if !defined(MAKE_LUAC)" >> ${OUTFILE}
    echo "#if defined(LUA_INCLUDE_LIBGLM)" >> ${OUTFILE} # GLM Binding: @WIP
      cat ${LUA_DIR}/libs/glm-binding/ext/vector_extensions.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/ext/matrix_extensions.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/ext/quat_extensions.hpp >> ${OUTFILE}

      cat ${LUA_DIR}/libs/glm-binding/ext/geom/setup.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/ext/geom/aabb.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/ext/geom/triangle.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/ext/geom/line.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/ext/geom/linesegment.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/ext/geom/plane.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/ext/geom/polygon.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/ext/geom/ray.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/ext/geom/sphere.hpp >> ${OUTFILE}

      cat ${LUA_DIR}/libs/glm-binding/allocator.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/bindings.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/iterators.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/api.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/geom.hpp >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/lglmlib.hpp >> ${OUTFILE}
    echo "#endif /* LUA_INCLUDE_LIBGLM */" >> ${OUTFILE}

    echo "/* standard library  -- not used by luac */" >> ${OUTFILE}
    cat ${LUA_DIR}/lbaselib.c >> ${OUTFILE}
    cat ${LUA_DIR}/lcorolib.c >> ${OUTFILE}
    cat ${LUA_DIR}/ldblib.c >> ${OUTFILE}
    cat ${LUA_DIR}/liolib.c >> ${OUTFILE}
    cat ${LUA_DIR}/lmathlib.c >> ${OUTFILE}
    cat ${LUA_DIR}/loadlib.c >> ${OUTFILE}
    cat ${LUA_DIR}/loslib.c >> ${OUTFILE}
    cat ${LUA_DIR}/lstrlib.c >> ${OUTFILE}
    cat ${LUA_DIR}/ltablib.c >> ${OUTFILE}
    cat ${LUA_DIR}/lutf8lib.c >> ${OUTFILE}
    echo "#if defined(LUA_INCLUDE_LIBGLM)" >> ${OUTFILE}
      cat ${LUA_DIR}/libs/glm-binding/lglmlib.cpp >> ${OUTFILE}
    echo "#endif /* LUA_INCLUDE_LIBGLM */" >> ${OUTFILE}
    cat ${LUA_DIR}/linit.c >> ${OUTFILE}
  echo "#endif /* MAKE_LUAC */" >> ${OUTFILE}
echo "#endif /* LUA_IMPL */" >> ${OUTFILE}

echo "#if defined(MAKE_LUA)" >> ${OUTFILE}
  echo "/* lua */" >> ${OUTFILE}
  cat ${LUA_DIR}/lua.c >> ${OUTFILE}
echo "#endif /* MAKE_LUA */" >> ${OUTFILE}

echo "#if defined(MAKE_LUAC)" >> ${OUTFILE}
  echo "/* luac */" >> ${OUTFILE}
  cat ${LUA_DIR}/luac.c >> ${OUTFILE}
echo "#endif /* MAKE_LUAC */" >> ${OUTFILE}

################################################################################
# Ensure includes are corrected:
#   - ljumptab.h and lglmlib_reg.hpp are embedded in code.
#   - lua.hpp, .lglmhpp, and lglm_string.hpp are used by the binding library.
#   - All other includes are prepended prior to any other code.
################################################################################

sed -i \
  -e "/#include \"ljumptab.h\"/r ${LUA_DIR}/ljumptab.h" \
  -e "/#include \"lglmlib_reg.hpp\"/r ${LUA_DIR}/libs/glm-binding/lglmlib_reg.hpp" \
  -e 's/#include <lua.hpp>/\/\*#include <lua.hpp>\*\//' \
  -e 's/#include <lglm.hpp>/\/\*#include <lglm.hpp>\*\//' \
  -e 's/#include <lglm_string.hpp>/\/\*#include <lglm_string.hpp>\*\//' \
  -e 's/#include "\([^"]*\)"/\/\*#include "\1"\*\//' ${OUTFILE}

################################################################################
# Licensing
################################################################################

cat << EOF >> ${OUTFILE}
/******************************************************************************
* LuaGLM
* Copyright (C) 2020 - gottfriedleibniz
\******************************************************************************
* OpenGL Mathematics (GLM)
* Copyright (C) 2005 - G-Truc Creation
\******************************************************************************
* Lua
* Copyright (C) 1994-2021 Lua.org, PUC-Rio.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/
EOF
