/*
** $Id: lgrit.h $
** Internal definitions for vector objects
** See Copyright Notice in lua.h
*/


#ifndef lgrit_h
#define lgrit_h

#include "llimits.h"
#include "lua.h"
#include "lobject.h"
#include "ltm.h"
#include "lgrit_lib.h"

#define V2_EQ(V, V2) (V_ISEQUAL((V).x, (V2).x) && V_ISEQUAL((V).y, (V2).y))
#define V3_EQ(V, V2) (V_ISEQUAL((V).z, (V2).z) && V2_EQ(V, V2))
#define V4_EQ(V, V2) (V_ISEQUAL((V).w, (V2).w) && V3_EQ(V, V2))

/*
** {==================================================================
** Vector Math
** ===================================================================
*/

/* Individual functions for vector length. */
LUAI_FUNC lua_Number luaVec_length2 (const lua_Float4 v2);
LUAI_FUNC lua_Number luaVec_length3 (const lua_Float4 v3);
LUAI_FUNC lua_Number luaVec_length4 (const lua_Float4 v4);

/* Creates a rotation which rotates angle degrees around axis. */
LUAI_FUNC int luaVec_angleaxis (const lua_Float4 v3, lua_VecF angle, lua_Float4 *q);

/* Returns the angle in degrees between two rotations a and b. */
LUAI_FUNC int luaVec_angle (const lua_Float4 a, const lua_Float4 b, lua_Float4 *q);

/* Converts a rotation to angle-axis representation (angles in degrees). */
LUAI_FUNC lua_Number luaVec_axisangle (const lua_Float4 v);

/* Converts a rotation to angle-axis representation (angles in degrees). */
LUAI_FUNC int luaVec_axis (const lua_Float4 v, lua_Float4 *r);

/* lmathlib compatbility */

LUAI_FUNC int (luaVec_dot) (lua_State *L);
LUAI_FUNC int (luaVec_cross) (lua_State *L);
LUAI_FUNC int (luaVec_inv) (lua_State *L);
LUAI_FUNC int (luaVec_norm) (lua_State *L);
LUAI_FUNC int (luaVec_slerp) (lua_State *L);

LUAI_FUNC int (luaVec_abs) (lua_State *L);
LUAI_FUNC int (luaVec_sin) (lua_State *L);
LUAI_FUNC int (luaVec_cos) (lua_State *L);
LUAI_FUNC int (luaVec_tan) (lua_State *L);
LUAI_FUNC int (luaVec_asin) (lua_State *L);
LUAI_FUNC int (luaVec_acos) (lua_State *L);
LUAI_FUNC int (luaVec_atan) (lua_State *L);
LUAI_FUNC int (luaVec_floor) (lua_State *L);
LUAI_FUNC int (luaVec_ceil) (lua_State *L);
LUAI_FUNC int (luaVec_fmod) (lua_State *L);
LUAI_FUNC int (luaVec_sqrt) (lua_State *L);
LUAI_FUNC int (luaVec_log) (lua_State *L);
LUAI_FUNC int (luaVec_exp) (lua_State *L);
LUAI_FUNC int (luaVec_deg) (lua_State *L);
LUAI_FUNC int (luaVec_rad) (lua_State *L);
LUAI_FUNC int (luaVec_min) (lua_State *L);
LUAI_FUNC int (luaVec_max) (lua_State *L);
LUAI_FUNC int (luaVec_clamp) (lua_State *L);

#if defined(LUA_COMPAT_MATHLIB)
LUAI_FUNC int (luaV_sinh) (lua_State *L);
LUAI_FUNC int (luaV_cosh) (lua_State *L);
LUAI_FUNC int (luaV_tanh) (lua_State *L);
LUAI_FUNC int (luaV_pow) (lua_State *L);
LUAI_FUNC int (luaV_log10) (lua_State *L);
#if defined(LUA_C99_MATHLIB)
LUAI_FUNC int luaV_asinh (lua_State *L);
LUAI_FUNC int luaV_acosh (lua_State *L);
LUAI_FUNC int luaV_atanh (lua_State *L);
LUAI_FUNC int luaV_cbrt (lua_State *L);
LUAI_FUNC int luaV_erf (lua_State *L);
LUAI_FUNC int luaV_erfc (lua_State *L);
LUAI_FUNC int luaV_exp2 (lua_State *L);
LUAI_FUNC int luaV_expm1 (lua_State *L);
LUAI_FUNC int luaV_gamma (lua_State *L);
LUAI_FUNC int luaV_lgamma (lua_State *L);
LUAI_FUNC int luaV_log1p (lua_State *L);
LUAI_FUNC int luaV_logb (lua_State *L);
LUAI_FUNC int luaV_nearbyint (lua_State *L);
LUAI_FUNC int luaV_round (lua_State *L);
LUAI_FUNC int luaV_trunc (lua_State *L);

LUAI_FUNC int luaV_isfinite (lua_State *L);
LUAI_FUNC int luaV_isinf (lua_State *L);
LUAI_FUNC int luaV_isnan (lua_State *L);
LUAI_FUNC int luaV_isnormal (lua_State *L);

LUAI_FUNC int luaV_fdim (lua_State *L);
LUAI_FUNC int luaV_hypot (lua_State *L);
LUAI_FUNC int luaV_scalbn (lua_State *L);
LUAI_FUNC int luaV_copysign (lua_State *L);
LUAI_FUNC int luaV_nextafter (lua_State *L);
LUAI_FUNC int luaV_remainder (lua_State *L);
#endif  /* LUA_C99_MATHLIB */
#endif

/* }================================================================== */

/*
** {==================================================================
** Object
** ===================================================================
*/

/* Constructors */

LUAI_FUNC int lua_vectorN (lua_State *L);
LUAI_FUNC int lua_vector2 (lua_State *L);
LUAI_FUNC int lua_vector3 (lua_State *L);
LUAI_FUNC int lua_vector4 (lua_State *L);
LUAI_FUNC int lua_quat (lua_State *L);

/* rawget variant for vector types */
LUAI_FUNC int luaVec_rawget (lua_State *L, const lua_Float4 *v, int vdims, TValue *key);

/* rawgeti variant for vector types */
LUAI_FUNC int luaVec_rawgeti (lua_State *L, const lua_Float4 *v, int vdims, lua_Integer n);

/* getfield/auxgetstr variant for vector types */
LUAI_FUNC int luavec_getstr (lua_State *L, const lua_Float4 *v, int vdims, const char *k);

/*
** Pops a key from the stack and pushes a <key, value> pair from the vector at
** the given stack index, the "next" pair after the given key.
**
** If there are no more elements in the vector, then returns 0 and pushes
** nothing.
*/
LUAI_FUNC int luaVec_next (lua_State *L, const lua_Float4 *v, int vdims, StkId key);

/* converts a vector to a string. */
LUAI_FUNC int luaVec_tostr (char *buff, size_t len, const lua_Float4 v, int variant);

/* Parse the string object and return the number of dimensions to the vector. */
LUAI_FUNC int luaVec_pullstring (lua_State *L, const TValue *o, lua_Float4 *sink);

/*
** Attempt to parse the provided table as a vector, i.e., check if the table has
** 'x', 'y', 'z', and 'w' fields and if those values are numeric types. If 'v'
** is not NULL, then the parsed contents are stored within the provided Float4.
**
** Returning the number of valid vector dimensions (bounded by the first
** dimension that is nil or not-numeric) the table has.
*/
LUAI_FUNC int luaVec_parse (lua_State* L, const TValue *o, lua_Float4 *v);

/*
** Unpack an individual vector and place its contents to onto the Lua stack,
** returning the number of elements (i.e., dimensions of vector).
*/
LUAI_FUNC int lua_unpackvec (lua_State *L);

/* Number of dimensions associated with the vector object */
static LUA_INLINE int luaVec_dimensions (const TValue *o) {
  const int dims = (rawtt(o) & 0x30) >> 4;  /* variant bits 4-5 */
  return (dims < 3) ? (2 + dims) : 4;  /* quat uses 3rd bit. */
}

/* }================================================================== */

/*
** {==================================================================
** LVM & LTM
** ===================================================================
*/

/* */
LUAI_FUNC int luaVec_trybinTM (lua_State *L, const TValue *p1, const TValue *p2, StkId res, TMS event);

/* Place the magnitude of the vector (o) at the specified stack index (ra) */
LUAI_FUNC void luaVec_objlen (lua_State *L, StkId ra, const TValue *o);

/*
** Access the contents of a vector type through string-indexing. Returning 1 if
** the TValue has been parsed & the StkId has been set.
*/
LUAI_FUNC void luaVec_getstring (lua_State *L, const TValue *t, const char *skey, TValue *key, StkId val);

/*
** Access the contents of a vector type through int-indexing, x = 1, y = 2,
** z = 3, w = 4. This function does not treat numeric TValue's as an
** implicit vector1, and will throw an error.
**
** Returning 1 if the TValue has been parsed & the StkId has been set.
*/
LUAI_FUNC void luaVec_getint (lua_State *L, const TValue *t, const lua_Integer key, TValue *pkey, StkId val);

/* }================================================================== */

#endif
