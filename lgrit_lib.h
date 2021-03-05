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
#include "lauxlib.h"

/*
** lobject.c:
** Maximum length of the conversion of a number to a string. Must be
** enough to accommodate both LUA_INTEGER_FMT and LUA_NUMBER_FMT.
** (For a long long int, this is 19 digits plus a sign and a final '\0',
** adding to 21. For a long double, it can go to a sign, 33 digits,
** the dot, an exponent letter, an exponent sign, 5 exponent digits,
** and a final '\0', adding to 43.)
*/
#define ORIG_MAXNUMBER2STR 44

/*
** Value changed from:
**   #define LUAI_MAXFLOAT2STR 16
**   #define LUAI_MAXVECTOR42STR (7 + 4*(1+LUAI_MAXFLOAT2STR+1))
** To something a bit more robust.
*/
#define LUAI_MAXVECTORSTR (7 + 4 * (1 + ORIG_MAXNUMBER2STR + 1))

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
** vector variants exposed in the library to simplify the internal/external
** translation between vector-types.
**
** NOTE: LUA_VVECTOR1 is the implicit vector-type (not enough variant bits) that
** is functionally equivalent to a LUA_TNUMBER. Therefore, ensure LUA_VVECTOR1
** is be equivalent to LUA_VNUMFLT internally.
*/
#define LUA_VVECTOR1 (LUA_TNUMBER | (1 << 4))
#define LUA_VVECTOR2 (LUA_TVECTOR | (0 << 4))
#define LUA_VVECTOR3 (LUA_TVECTOR | (1 << 4))
#define LUA_VVECTOR4 (LUA_TVECTOR | (2 << 4))
#define LUA_VQUAT    (LUA_TVECTOR | (3 << 4))

/*
** {==================================================================
** Base
** ===================================================================
*/

#define V_NOTABLE 0x0  /* Only explicit vectors can be tovector'd */
#define V_PARSETABLE 0x1  /* Attempt to parse a table object as a vector. */
#define V_NONUMBER 0x2 /* Ignore lua_Number == LUA_VVECTOR1 */

/* Returns the variant of the vector if it is indeed a vector, zero otherwise */
LUA_API int lua_isvector (lua_State *L, int idx, int flags);
LUA_API int lua_tovector (lua_State *L, int idx, int flags, lua_Float4 *vector);
LUA_API void lua_pushvector (lua_State *L, lua_Float4 f4, int variant);

/* Returns the name of the type encoded by the (vector-variant) value tp */
LUA_API const char *lua_vectypename (lua_State *L, int tp);

/* Return the label associated with a given vector dimension. */
LUA_API const char *lua_dimension_label (lua_State *L, int idx);

/* Push a string representing the vector object on top of the stack. */
LUA_API const char *lua_pushvecstring (lua_State *L, int idx);

/*
** Jenkins-hash the object at the provided index. String values are hashed,
** boolean and numeric values are casted to lua_Integer; otherwise, zero is
** returned.
**
** @PARAM ignore_case: A string value is hashed as-is. Otherwise, the lowercase
**  of each string character is computed then hashed.
**
** @TODO: Possibly consider allow the (potentially destructive) lua_tolstring.
*/
LUA_API lua_Integer lua_ToHash (lua_State *L, int idx, int ignore_case);

/* Number of dimensions associated with the vector object */
static LUA_INLINE int luaVec_dimensions (int rawtt) {
  const int dims = (rawtt & 0x30) >> 4;  /* variant bits 4-5 */
  return (dims < 3) ? (2 + dims) : 4;  /* quat uses 3rd bit. */
}

/* }================================================================== */

/*
** {==================================================================
** Compatibility API
** ===================================================================
*/

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
#define _lua_checkv(L, I, F, V, T, ERR)          \
  do {                                           \
    if (lua_tovector((L), (I), (F), (V)) != (T)) \
      luaL_typeerror((L), (I), (ERR));           \
  } while (0)

#define lua_checkv1(L, I, F, V) _lua_checkv(L, I, F, V, LUA_VVECTOR1, LABEL_VECTOR1)
#define lua_checkv2(L, I, F, V) _lua_checkv(L, I, F, V, LUA_VVECTOR2, LABEL_VECTOR2)
#define lua_checkv3(L, I, F, V) _lua_checkv(L, I, F, V, LUA_VVECTOR3, LABEL_VECTOR3)
#define lua_checkv4(L, I, F, V) _lua_checkv(L, I, F, V, LUA_VVECTOR4, LABEL_VECTOR4)
#define lua_checkquat(L, I, F, V) _lua_checkv(L, I, F, V, LUA_VQUAT, LABEL_QUATERN)

/*
** Compatibility API, replaced by: a single lua_pushvector in lua.h. This macro
** avoids the use of designated initializers for C++ (e.g., MSVC C7555)
*/
#define _lua_pushvector(L, T, X, Y, Z, W)              \
  do {                                                 \
    /* Avoid use of designated initializers for C++ */ \
    lua_Float4 f4 = { (X), (Y), (Z), (W) };            \
    lua_pushvector((L), f4, (T));                      \
  } while (0)

#define lua_pushvector2(L, x, y) _lua_pushvector(L, LUA_VVECTOR2, x, y, V_ZERO, V_ZERO)
#define lua_pushvector3(L, x, y, z) _lua_pushvector(L, LUA_VVECTOR3, x, y, z, V_ZERO)
#define lua_pushvector4(L, x, y, z, w) _lua_pushvector(L, LUA_VVECTOR4, x, y, z, w)
#define lua_pushquat(L, w, x, y, z) _lua_pushvector(L, LUA_VQUAT, x, y, z, w)

/* }================================================================== */

#endif
