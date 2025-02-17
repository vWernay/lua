/*
** $Id: lgrit_lib.h $
** API definitions for grit-lua.
**
** All functions defined in this header are expected to have default linkage and
** interface with other Lua libraries. For simplicity, this header also
** maintains all LuaGLM API functions under the same constraints.
**
** See Copyright Notice in lua.h
*/

#ifndef lgritlib_h
#define lgritlib_h

#include <math.h>

#include "luaconf.h"
#include "lua.h"
#include "lauxlib.h"

/* Avoid old-style-cast for C++ libraries */
#if defined(__cplusplus)
  #define cast_vec(i) static_cast<lua_VecF>((i))
#else
  #define cast_vec(i) ((lua_VecF)(i))
#endif

/*
** {==================================================================
** LuaGLM C-API
** ===================================================================
*/

/* Constructors */

LUA_API int glmVec_vec (lua_State *L);
LUA_API int glmVec_vec1 (lua_State *L);
LUA_API int glmVec_vec2 (lua_State *L);
LUA_API int glmVec_vec3 (lua_State *L);
LUA_API int glmVec_vec4 (lua_State *L);

LUA_API int glmVec_ivec (lua_State *L);
LUA_API int glmVec_ivec1 (lua_State *L);
LUA_API int glmVec_ivec2 (lua_State *L);
LUA_API int glmVec_ivec3 (lua_State *L);
LUA_API int glmVec_ivec4 (lua_State *L);

LUA_API int glmVec_bvec (lua_State *L);
LUA_API int glmVec_bvec1 (lua_State *L);
LUA_API int glmVec_bvec2 (lua_State *L);
LUA_API int glmVec_bvec3 (lua_State *L);
LUA_API int glmVec_bvec4 (lua_State *L);

LUA_API int glmMat_mat (lua_State *L);
LUA_API int glmMat_mat2x2 (lua_State *L);
LUA_API int glmMat_mat2x3 (lua_State *L);
LUA_API int glmMat_mat2x4 (lua_State *L);
LUA_API int glmMat_mat3x2 (lua_State *L);
LUA_API int glmMat_mat3x3 (lua_State *L);
LUA_API int glmMat_mat3x4 (lua_State *L);
LUA_API int glmMat_mat4x2 (lua_State *L);
LUA_API int glmMat_mat4x3 (lua_State *L);
LUA_API int glmMat_mat4x4 (lua_State *L);

LUA_API int glmVec_qua (lua_State *L);

/* Returns the name of the type encoded by the vector variant */
LUA_API const char *glm_typename (lua_State *L, int idx);

/* Push a string representing the vector/matrix object on top of the stack. */
LUA_API const char *glm_pushstring (lua_State *L, int idx);

/*
** Unpack an individual vector and place its contents to onto the Lua stack,
** returning the number of elements (i.e., dimensions of vector).
*/
LUA_API int glm_unpack_vector (lua_State *L, int idx);

/*
** Unpack an the individual column-vectors of the given Matrix and place them
** onto the Lua stack, returning the number of elements, dimensions of vector,
** placed on the stack.
*/
LUA_API int glm_unpack_matrix (lua_State *L, int idx);

/*
** Jenkins-hash the object at the provided index. String values are hashed,
** boolean and numeric values are casted to lua_Integer; otherwise, zero is
** returned.
**
** ignore_case: A string value is hashed as-is. Otherwise, the lowercase of each
**  string character is computed then hashed.
**
** @TODO: Possibly consider allow the potentially destructive lua_tolstring.
*/
LUA_API lua_Integer glm_tohash (lua_State *L, int idx, int ignore_case);

/* }================================================================== */

/*
** {==================================================================
** @DEPRECATED: grit-lua API
** ===================================================================
*/

/* These functions are defined in lua.h of grit-lua */

LUA_API int lua_isvector2 (lua_State *L, int idx);
LUA_API int lua_isvector3 (lua_State *L, int idx);
LUA_API int lua_isvector4 (lua_State *L, int idx);
LUA_API int lua_isquat (lua_State *L, int idx);

LUA_API void lua_checkvector2 (lua_State *L, int idx, lua_VecF *x, lua_VecF *y);
LUA_API void lua_checkvector3 (lua_State *L, int idx, lua_VecF *x, lua_VecF *y, lua_VecF *z);
LUA_API void lua_checkvector4 (lua_State *L, int idx, lua_VecF *x, lua_VecF *y, lua_VecF *z, lua_VecF *w);
LUA_API void lua_checkquat (lua_State *L, int idx, lua_VecF *w, lua_VecF *x, lua_VecF *y, lua_VecF *z);

LUA_API void lua_pushvector2 (lua_State *L, lua_VecF x, lua_VecF y);
LUA_API void lua_pushvector3 (lua_State *L, lua_VecF x, lua_VecF y, lua_VecF z);
LUA_API void lua_pushvector4 (lua_State *L, lua_VecF x, lua_VecF y, lua_VecF z, lua_VecF w);
LUA_API void lua_pushquat (lua_State *L, lua_VecF w, lua_VecF x, lua_VecF y, lua_VecF z);

/* }================================================================== */

/*
** {==================================================================
** @DEPRECATED: Extended grit-lua API
** ===================================================================
*/

/*
** vector variants exposed in the library to simplify the internal/external
** translation between vector-types. (grit-lua compatibility)
*/
#if !defined(LUA_VVECTOR3)
#define LUA_VVECTOR1 (LUA_TNUMBER | (1 << 4))
#define LUA_VVECTOR2 (LUA_TVECTOR | (0 << 4))
#define LUA_VVECTOR3 (LUA_TVECTOR | (1 << 4))
#define LUA_VVECTOR4 (LUA_TVECTOR | (2 << 4))
#define LUA_VQUAT (LUA_TVECTOR | (3 << 4))
#endif

/* Returns the length of the vector if it is indeed a vector, zero otherwise */
LUA_API int lua_isvector (lua_State *L, int idx);
LUA_API int lua_tovector (lua_State *L, int idx, lua_Float4 *vector);
LUA_API void lua_pushvector (lua_State *L, lua_Float4 f4, int variant);
LUA_API void lua_pushquatf4 (lua_State *L, lua_Float4 f4);

/* Returns true if the object at the given index is a matrix, storing its
** dimensions in size & secondary. These are extensions to the grit-lua API */
LUA_API int lua_ismatrix (lua_State *L, int idx, int *dimensions);
LUA_API int lua_tomatrix (lua_State *L, int idx, lua_Mat4 *matrix);
LUA_API void lua_pushmatrix (lua_State *L, const lua_Mat4 *matrix);

/* }================================================================== */

/*
** {==================================================================
** @DEPRECATED: grit-lua base library compatibility
** ===================================================================
*/

/*
** Jenkins-hash the object at the provided index. String values are hashed,
** boolean and numeric values are casted to lua_Integer; otherwise, zero is
** returned.
*/
LUA_API lua_Integer lua_ToHash (lua_State *L, int idx, int ignore_case);

/*
** Returns the dot product of x and y, i.e., result = x * y.
**   T glm::dot(qua<T, Q> const &x, qua<T, Q> const &y)
**   T glm::dot(vec<L, T, Q> const &x, vec<L, T, Q> const &y)
*/
LUA_API int glmVec_dot (lua_State *L);

/*
** Returns the cross product of x and y.
**   qua<T, Q> glm::cross(qua<T, Q> const &x, qua<T, Q> const &y)
**   vec<3, T, Q> glm::cross(qua<T, Q> const &x, vec<3, T, Q> const &y)
**   vec<3, T, Q> glm::cross(vec<3, T, Q> const &x, vec<3, T, Q> const &y)
**   vec<3, T, Q> glm::cross(vec<3, T, Q> const &x, qua<T, Q> const &y)
**   T glm::cross(vec<2, T, Q> const &x, vec<2, T, Q> const &y)
*/
LUA_API int glmVec_cross (lua_State *L);

/*
** Returns the m/q inverse.
**   qua<T, Q> glm::inverse(qua<T, Q> const &q)
**   mat<C, R, T, Q> glm::inverse(mat<C, R, T, Q> const &m)
*/
LUA_API int glmVec_inverse (lua_State *L);

/*
** Returns a vector in the same direction as x but with length of 1.
**   qua<T, Q> glm::normalize(qua<T, Q> const &x)
**   vec<L, T, Q> glm::normalize(vec<L, T, Q> const &x)
*/
LUA_API int glmVec_normalize (lua_State *L);

/*
** Spherical linear interpolation of two vectors/quaternions.
**   qua<T, Q> glm::slerp(qua<T, Q> const &x, qua<T, Q> const &y, T a)
**   vec<3, T, Q> glm::slerp(vec<3, T, Q> const &x, vec<3, T, Q> const &y, T const &a)
*/
LUA_API int glmVec_slerp (lua_State *L);

/* grit-lua math library extension */
LUA_API int glmVec_clamp (lua_State *L);

/* }================================================================== */

#endif
