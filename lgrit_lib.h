/*
** $Id: lgrit_lib.h $
** API & Auxiliary definitions for gritLua.
** See Copyright Notice in lua.h
*/

#ifndef lgritlib_h
#define lgritlib_h

#include <math.h>
#include "luaconf.h"
#include "lua.h"

/* Avoid old-style-cast for C++ libraries */
#if defined(__cplusplus)
  #define cast_vec(i) static_cast<lua_VecF>((i))
#else
  #define cast_vec(i) ((lua_VecF)(i))
#endif

#define V_ZERO (cast_vec(0.0))
#define V_HALF (cast_vec(0.5))
#define V_ONE (cast_vec(1.0))
#define V_TWO (cast_vec(2.0))
#define V_PI cast_vec(3.141592653589793238462643383279502884)
#define V_ISZERO(a) (l_vecop(fabs)((a)) <= LUA_VEC_NUMBER_EPS)
#define V_ISEQUAL(a, b) (V_ISZERO((a) - (b)) || ((a) == (b)))

/*
** {==================================================================
** API
** ===================================================================
*/

#define LABEL_INTEGER "integer"
#define LABEL_NUMBER "number"
#define LABEL_VECTOR "vector"
#define LABEL_VECTOR1 "vector1"
#define LABEL_VECTOR2 "vector2"
#define LABEL_VECTOR3 "vector3"
#define LABEL_VECTOR4 "vector4"
#define LABEL_QUATERN "quat"
#define LABEL_ALL "number or vector type"

/*
** Compatibility API: check if the object at the given index is a vector of a
** specific variant.
*/
#define lua_isvector1(L, I, F) (lua_isvector((L), (I), (F)) == LUA_VVECTOR1)
#define lua_isvector2(L, I, F) (lua_isvector((L), (I), (F)) == LUA_VVECTOR2)
#define lua_isvector3(L, I, F) (lua_isvector((L), (I), (F)) == LUA_VVECTOR3)
#define lua_isvector4(L, I, F) (lua_isvector((L), (I), (F)) == LUA_VVECTOR4)
#define lua_isquat(L, I, F) (lua_isvector((L), (I), V_NOTABLE) == LUA_VQUAT \
                             || (((F)&V_PARSETABLE) != 0 && lua_isvector((L), (I), (F)) == LUA_VVECTOR4))

/*
** Compatibility API: Populate a Float4 of an explicit type, throwing an error
** otherwise.
*/
#define _lua_checkv(L, I, F, V, T, ERR)        \
  LUA_MLM_BEGIN                                \
  if (lua_tovector((L), (I), (F), (V)) != (T)) \
    luaL_typeerror((L), (I), (ERR));           \
  LUA_MLM_END

#define lua_checkv1(L, I, F, V) _lua_checkv(L, I, F, V, LUA_VVECTOR1, LABEL_VECTOR1)
#define lua_checkv2(L, I, F, V) _lua_checkv(L, I, F, V, LUA_VVECTOR2, LABEL_VECTOR2)
#define lua_checkv3(L, I, F, V) _lua_checkv(L, I, F, V, LUA_VVECTOR3, LABEL_VECTOR3)
#define lua_checkv4(L, I, F, V) _lua_checkv(L, I, F, V, LUA_VVECTOR4, LABEL_VECTOR4)
#define lua_checkquat(L, I, F, V) _lua_checkv(L, I, F, V, LUA_VQUAT, LABEL_QUATERN)

/*
** Compatibility API, replaced by: a single lua_pushvector in lua.h. This macro
** avoids the use of designated initializers for C++ (e.g., MSVC C7555)
*/
#define _lua_pushvector(L, T, X, Y, Z, W)            \
  LUA_MLM_BEGIN                                      \
  /* Avoid use of designated initializers for C++ */ \
  lua_Float4 f4 = { (X), (Y), (Z), (W) };            \
  lua_pushvector((L), f4, (T));                      \
  LUA_MLM_END

#define lua_pushvector2(L, x, y) _lua_pushvector(L, LUA_VVECTOR2, x, y, V_ZERO, V_ZERO)
#define lua_pushvector3(L, x, y, z) _lua_pushvector(L, LUA_VVECTOR3, x, y, z, V_ZERO)
#define lua_pushvector4(L, x, y, z, w) _lua_pushvector(L, LUA_VVECTOR4, x, y, z, w)
#define lua_pushquat(L, w, x, y, z) _lua_pushvector(L, LUA_VQUAT, x, y, z, w)

/* }================================================================== */

/*
** {==================================================================
** Base
** ===================================================================
*/

/* Number of dimensions associated with the vector type */
LUA_API int lua_dimensions_count (lua_State *L, int tp);

/* Returns the name of the type encoded by the (vector-variant) value tp */
LUA_API const char *lua_vectypename (lua_State *L, int tp);

/* Return the label associated with a given vector dimension. */
LUA_API const char *lua_dimension_label (lua_State *L, int idx);

/* Push a string representing the vector object on top of the stack. */
LUA_API const char *lua_pushvecstring (lua_State *L, int idx);

/* TODO: Change API to use lua_Unsigned */
/* one_at_a_time: http://www.burtleburtle.net/bob/hash/doobs.html */
LUA_API lua_Integer lua_ToHash (lua_State *L, int idx);

/* }================================================================== */

#endif
