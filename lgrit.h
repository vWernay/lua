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

#define V2_EQ(V, V2) luai_numeq((V).x, (V2).x) && luai_numeq((V).y, (V2).y)
#define V3_EQ(V, V2) luai_numeq((V).z, (V2).z) && V2_EQ(V, V2)
#define V4_EQ(V, V2) luai_numeq((V).w, (V2).w) && V3_EQ(V, V2)

/*
** Vector Object Math
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

/*
** {==================================================================
** Object
** ===================================================================
*/

/* Place the magnitude of the vector (o) at the specified stack index (ra) */
LUAI_FUNC void (luaVec_objlen) (lua_State *L, StkId ra, const TValue *o);

/* converts a vector to a string. */
LUAI_FUNC int (luaVec_tostr) (char *buff, size_t len, const lua_Float4 v,
                                                                   int variant);

/* Parse the string object and return the number of dimensions to the vector. */
LUAI_FUNC int (luaVec_pullstring) (lua_State *L, const TValue *o,
                                                              lua_Float4 *sink);

/* */
LUAI_FUNC int luaVec_trybinTM (lua_State *L, const TValue *p1, const TValue *p2,
                                                          StkId res, TMS event);

/* }================================================================== */

/*
** {==================================================================
** LVM
** ===================================================================
*/

/*
** Access the contents of a vector type through string-indexing. Returning 1 if
** the TValue has been parsed & the StkId has been set.
*/
LUAI_FUNC void (luaVec_getstring) (lua_State *L, const TValue* t,
                                      const char* skey, TValue* key, StkId val);

/*
** Access the contents of a vector type through int-indexing, x = 1, y = 2,
** z = 3, w = 4. This function does not treat numeric TValue's as an
** implicit vector1, and will throw an error.
**
** Returning 1 if the TValue has been parsed & the StkId has been set.
*/
LUAI_FUNC void (luaVec_getint) (lua_State *L, const TValue *t, const
                                      lua_Integer key, TValue* pkey, StkId val);

/*
** Resolves and canonicalizes rel (in the context of the dir part of file).
**
** (1) Handles .. and .
** (2) Breaks up both file and rel into a list of dirs.
** (3) Chops the filename from file.
** (4) Compresses this to handle .. and .
** (5) Reconstitutes into an absolute path.
*/
LUAI_FUNC TString *(resolve_absolute_path) (lua_State *L, const char *file,
                                                               const char *rel);

/* }================================================================== */

#endif
