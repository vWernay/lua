/*
** TODO: Both core & library functions reside in this file. As this codebase
**       develops, it'll eventually need to be split.
*/
#define lgrit_c
#define LUA_LIB

#include <math.h>
#include <string.h>
#include <locale.h>

#include "lua.h"
#include "luaconf.h"
#include "ldebug.h"
#include "lobject.h"
#include "lauxlib.h"
#include "lstring.h"
#include "ltable.h"
#include "lvm.h"
#include "lgrit.h"
#include "lgrit_lib.h"

#define NAME_VECTOR2 "vector2"
#define NAME_VECTOR3 "vector3"
#define NAME_VECTOR4 "vector4"
#define NAME_QUAT    "quat"

/* Dimension strings: c_dims + \0, must be consistent with luaVec_swizzle */
static const char* const dims[] = { "x", "y", "z", "w" };

/* If you lie to the compiler, it will get its revenge. */
static void luaV_assignf4 (lua_Float4 *to, int index, lua_VecF n) {
  switch (index) {
    case 0: to->x = n; break;
    case 1: to->y = n; break;
    case 2: to->z = n; break;
    case 3: to->w = n; break;
    default: break;
  }
}

static lua_VecF luaV_getf4 (const lua_Float4 *to, int index) {
  switch (index) {
    case 0: return to->x;
    case 1: return to->y;
    case 2: return to->z;
    case 3: return to->w;
    default:
      return V_ZERO;
  }
}

static int luaV_checkdimension (const char *v) {
  int i;
  for (i = 0; i < 4; i++) {
    if (strcmp(dims[i], v) == 0)
      return i;
  }
  return -1;
}

static int luaVec_swizzle (const char *key, const lua_Float4 *from, int from_sz,
                                                               lua_Float4 *to) {
  int counter = 0;
  while (key[counter] != '\0') {
    switch (key[counter]) {
      case 'x': if (from_sz < 1) { return 0; } luaV_assignf4(to, counter, from->x); break;
      case 'y': if (from_sz < 2) { return 0; } luaV_assignf4(to, counter, from->y); break;
      case 'z': if (from_sz < 3) { return 0; } luaV_assignf4(to, counter, from->z); break;
      case 'w': if (from_sz < 4) { return 0; } luaV_assignf4(to, counter, from->w); break;
      default: return 0;
    }
    if ((++counter) > 4)
      return 0;
  }
  return counter;
}

/*
** {==================================================================
** API
** ===================================================================
*/

#define LUAV_TYPE(T) return lua_isvector(L, idx, flags) == (T);
LUA_API int lua_isvector1 (lua_State *L, int idx, int flags) { LUAV_TYPE(LUA_VNUMFLT); }
LUA_API int lua_isvector2 (lua_State *L, int idx, int flags) { LUAV_TYPE(LUA_VVECTOR2); }
LUA_API int lua_isvector3 (lua_State *L, int idx, int flags) { LUAV_TYPE(LUA_VVECTOR3); }
LUA_API int lua_isvector4 (lua_State *L, int idx, int flags) { LUAV_TYPE(LUA_VVECTOR4); }
LUA_API int lua_isquat (lua_State *L, int idx, int flags) {
  return lua_isvector(L, idx, V_NOTABLE) == LUA_VQUAT ||
    (flags && lua_isvector(L, idx, flags) == LUA_VVECTOR4);
}

#define LUAV_CHECK(T, ERR) if (lua_tovector(L, idx, flags, v) != (T)) luaL_typeerror(L, idx, (ERR));
void lua_checkv1 (lua_State *L, int idx, int flags, lua_Float4 *v) { LUAV_CHECK(LUA_VNUMFLT, "vector1") }
void lua_checkv2 (lua_State *L, int idx, int flags, lua_Float4 *v) { LUAV_CHECK(LUA_VVECTOR2, "vector2") }
void lua_checkv3 (lua_State *L, int idx, int flags, lua_Float4 *v) { LUAV_CHECK(LUA_VVECTOR3, "vector3") }
void lua_checkv4 (lua_State *L, int idx, int flags, lua_Float4 *v) { LUAV_CHECK(LUA_VVECTOR4, "vector4") }
void lua_checkquat (lua_State *L, int idx, int flags, lua_Float4 *v) { LUAV_CHECK(LUA_VQUAT, "quat") }

/* API Compatibility */

LUA_API void lua_pushvector2 (lua_State *L, lua_VecF x, lua_VecF y) {
  lua_Float4 f4 = { .x = x, .y = y, .z = V_ZERO, .w = V_ZERO };
  lua_pushvector(L, f4, LUA_VVECTOR2);
}

LUA_API void lua_pushvector3 (lua_State *L, lua_VecF x, lua_VecF y, lua_VecF z) {
  lua_Float4 f4 = { .x = x, .y = y, .z = z, .w = V_ZERO };
  lua_pushvector(L, f4, LUA_VVECTOR3);
}

LUA_API void lua_pushvector4 (lua_State *L, lua_VecF x, lua_VecF y, lua_VecF z, lua_VecF w) {
  lua_Float4 f4 = { .x = x, .y = y, .z = z, .w = w };
  lua_pushvector(L, f4, LUA_VVECTOR4);
}

LUA_API void lua_pushquat (lua_State *L, lua_VecF w, lua_VecF x, lua_VecF y, lua_VecF z) {
  lua_Float4 f4 = { .x = x, .y = y, .z = z, .w = w };
  lua_pushvector(L, f4, LUA_VQUAT);
}

/* }================================================================== */

/*
** {==================================================================
** Table API
** ===================================================================
*/

static int vectable_getstr (lua_State *L, const TValue *t, const char *k) {
  const TValue *slot;
  TString *str = luaS_new(L, k);
  lua_assert(str->tt == LUA_VSHRSTR);
  if (luaV_fastget(L, t, str, slot, luaH_getshortstr)) {
    setobj2s(L, L->top++, slot);
  }
  else {
    setsvalue2s(L, L->top++, str);
    luaV_finishget(L, t, s2v(L->top - 1), L->top - 1, slot);
  }
  return ttype(s2v(L->top - 1));
}

int luaVec_parse (lua_State* L, const TValue* o, lua_Float4 *v) {
  if (v != NULL) {  /* Ensure vector data is cleared */
    v->x = v->y = v->z = v->w = V_ZERO;
  }

  if (ttistable(o)) {
    int i, count = 0;
    luaL_checkstack(L, 4, NULL);  /* ensure table space */
    for (i = 0; i < 4; ++i) {
      if (vectable_getstr(L, o, dims[i]) == LUA_TNUMBER) {
        if (v != NULL)
          luaV_assignf4(v, i, cast_vec(nvalue(s2v(L->top - 1))));
        L->top--; count++;
      }
      else {
        L->top--; break;
      }
    }
    return count;
  }
  else if (ttisvector(o)) {
    if (v != NULL)
      *v = vvalue(o);
    return lua_dimensions_count(L, ttypetag(o));
  }
  return 0;
}

lua_Float4 luaVec_value (lua_State* L, const TValue* o) {
  if (ttisvector(o))
    return vvalue(o);
  else {
    lua_Float4 v = V_ZEROVEC;
    if (ttistable(o))
      luaVec_parse(L, o, &v);
    return v;
  }
}

/* }================================================================== */

/*
** {==================================================================
** Base
** ===================================================================
*/

static int luaB_vectorn (lua_State *L, int sz, lua_Float4 *input) {
  int counter = 0, i, isnum;
  for (i = 1; i <= lua_gettop(L); ++i) {
    if (lua_isnumber(L, i)) {
      if (counter + 1 <= sz)
        luaV_assignf4(input, counter++, cast_vec(lua_tonumberx(L, i, &isnum)));
      else
        return 0;
    }
    else if (lua_isvector(L, i, V_PARSETABLE)) {
      lua_Float4 f4;
      switch (lua_tovector(L, i, V_PARSETABLE, &f4)) {
        case LUA_VNUMFLT:
          if (counter + 1 > sz) return 0;
          luaV_assignf4(input, counter++, f4.x);
          break;
        case LUA_VVECTOR2:
          if (counter + 2 > sz) return 0;
          luaV_assignf4(input, counter++, f4.x);
          luaV_assignf4(input, counter++, f4.y);
          break;
        case LUA_VVECTOR3:
          if (counter + 3 > sz) return 0;
          luaV_assignf4(input, counter++, f4.x);
          luaV_assignf4(input, counter++, f4.y);
          luaV_assignf4(input, counter++, f4.z);
          break;
        case LUA_VVECTOR4:
        case LUA_VQUAT:
          if (counter + 4 > sz) return 0;
          luaV_assignf4(input, counter++, f4.x);
          luaV_assignf4(input, counter++, f4.y);
          luaV_assignf4(input, counter++, f4.z);
          luaV_assignf4(input, counter++, f4.w);
          break;
        default:
          return luaL_error(L, "unexpected vector type.");
      }
    }
    else {
      return luaL_error(L, "vector%d(...) argument %d had type %s", sz, i, lua_typename(L, lua_type(L, i)));
    }
  }
  return counter == sz;
}

LUA_API const char *lua_dimension_label (lua_State *L, int idx) {
  UNUSED(L);
  return (idx >= 1 && idx <= 4) ? dims[idx - 1] : NULL;
}

LUA_API int lua_dimensions_count (lua_State *L, int tp) {
  switch (tp) {
    case LUA_VNUMFLT: return 1;
    case LUA_VVECTOR2: return 2;
    case LUA_VVECTOR3: return 3;
    case LUA_VQUAT: case LUA_VVECTOR4: return 4;
    default:
      luaL_typeerror(L, tp, "vectortype");
      return 0;
  }
}

LUA_API const char *lua_vectypename (lua_State *L, int t) {
  switch (t) {
    case LUA_VVECTOR2: return NAME_VECTOR2;
    case LUA_VVECTOR3: return NAME_VECTOR3;
    case LUA_VVECTOR4: return NAME_VECTOR4;
    case LUA_VQUAT: return NAME_QUAT;
    default:
      luaL_typeerror(L, t, "vectortype");
      return "";
  }
}

LUA_API int lua_vectorN (lua_State *L) {
  lua_Float4 v = V_ZEROVEC;
  switch (lua_gettop(L)) {
    case 4:
      v.x = cast_vec(luaL_checknumber(L, 1));
      v.y = cast_vec(luaL_checknumber(L, 2));
      v.z = cast_vec(luaL_checknumber(L, 3));
      v.w = cast_vec(luaL_checknumber(L, 4));
      lua_pushvector(L, v, LUA_VVECTOR4);
      break;
    case 3:
      v.x = cast_vec(luaL_checknumber(L, 1));
      v.y = cast_vec(luaL_checknumber(L, 2));
      v.z = cast_vec(luaL_checknumber(L, 3));
      lua_pushvector(L, v, LUA_VVECTOR3);
      break;
    case 2:
      v.x = cast_vec(luaL_checknumber(L, 1));
      v.y = cast_vec(luaL_checknumber(L, 2));
      lua_pushvector(L, v, LUA_VVECTOR2);
      break;
    case 1:
      lua_pushnumber(L, luaL_checknumber(L, 1));
      break;
    default:
      return luaL_error(L, "vec(...) takes 1 to 4 number arguments only");
  }
  return 1;
}

LUA_API int lua_vector2 (lua_State *L) {
  lua_Float4 v;
  if (luaB_vectorn(L, 2, &v)) {
    lua_pushvector(L, v, LUA_VVECTOR2);
    return 1;
  }
  return luaL_error(L, "vector2(...) requires exactly 2 numbers");
}

LUA_API int lua_vector3 (lua_State *L) {
  lua_Float4 v;
  if (luaB_vectorn(L, 3, &v)) {
    lua_pushvector(L, v, LUA_VVECTOR3);
    return 1;
  }
  return luaL_error(L, "vector3(...) requires exactly 3 numbers");
}

LUA_API int lua_vector4 (lua_State *L) {
  lua_Float4 v;
  if (luaB_vectorn(L, 4, &v)) {
    lua_pushvector(L, v, LUA_VVECTOR4);
    return 1;
  }
  return luaL_error(L, "vector4(...) requires exactly 4 numbers");
}

LUA_API int lua_quat (lua_State *L) {
  lua_Float4 q = { .w = V_ONE, .x = V_ZERO, .y = V_ZERO, .z = V_ZERO };
  if (lua_gettop(L) == 4 && lua_isnumber(L, 1) && lua_isnumber(L, 2) &&
                            lua_isnumber(L, 3) && lua_isnumber(L, 4)) {
    q.w = cast_vec(lua_tonumber(L, 1));
    q.x = cast_vec(lua_tonumber(L, 2));
    q.y = cast_vec(lua_tonumber(L, 3));
    q.z = cast_vec(lua_tonumber(L, 4));
  }
  else if (lua_gettop(L) == 2 && lua_isnumber(L, 1) && lua_isvector3(L, 2, V_PARSETABLE)) {
    lua_Float4 v3;
    lua_VecF angle = cast_vec(lua_tonumber(L, 1));
    lua_checkv3(L, 2, V_PARSETABLE, &v3);
    if (!luaVec_angleaxis(v3, angle, &q)) {  /* Identity quaternion on fail */
      q.w = V_ONE; q.x = q.y = q.z = V_ZERO;
    }
  }
  else if (lua_gettop(L) == 2 && lua_isvector3(L, 1, V_PARSETABLE) && lua_isvector3(L, 2, V_PARSETABLE)) {
    lua_Float4 v, v2;
    lua_checkv3(L, 1, V_PARSETABLE, &v);
    lua_checkv3(L, 2, V_PARSETABLE, &v2);
    if (!luaVec_angle(v, v2, &q)) {
      q.w = V_ONE; q.x = q.y = q.z = V_ZERO;
    }
  }
  else {
    return luaL_error(L, "Invalid params, try quat(n,n,n,n) quat(n,v3) quat(v3,v3)");
  }

  lua_pushvector(L, q, LUA_VQUAT);
  return 1;
}

LUA_API int lua_unpackvec (lua_State *L) {
  luaL_checkstack(L, 4, "vector fields");  /* Ensure stack-space */
  if (lua_isinteger(L, 1))
    lua_pushinteger(L, luaL_checkinteger(L, 1));
  else if (lua_isnumber(L, 1))
    lua_pushnumber(L, luaL_checknumber(L, 1));
  else {
    lua_Float4 v;
    switch (lua_tovector(L, 1, V_NOTABLE, &v)) {
      case LUA_VNUMFLT:
        lua_pushnumber(L, cast_num(v.x));
        return 1;
      case LUA_VVECTOR2:
        lua_pushnumber(L, cast_num(v.x));
        lua_pushnumber(L, cast_num(v.y));
        return 2;
      case LUA_VVECTOR3:
        lua_pushnumber(L, cast_num(v.x));
        lua_pushnumber(L, cast_num(v.y));
        lua_pushnumber(L, cast_num(v.z));
        return 3;
      case LUA_VVECTOR4:
        lua_pushnumber(L, cast_num(v.x));
        lua_pushnumber(L, cast_num(v.y));
        lua_pushnumber(L, cast_num(v.z));
        lua_pushnumber(L, cast_num(v.w));
        return 4;
      case LUA_VQUAT:
        lua_pushnumber(L, cast_num(v.w));
        lua_pushnumber(L, cast_num(v.x));
        lua_pushnumber(L, cast_num(v.y));
        lua_pushnumber(L, cast_num(v.z));
        return 4;
      default:
        return luaL_error(L, "vunpack takes a number, integer, vector2, vector3, or vector4");
    }
  }
  return 1;
}

void luaVec_objlen (lua_State *L, StkId ra, const TValue *o) {
  /*
  ** For performance reasons, only use the __length metamethod if specified at
  ** compile time.
  */
#if defined(GRIT_META_LEN)
  const TValue *tm;
  if ((tm = luaT_gettmbyobj(L, o, TM_LEN))) {  /* metamethod */
    luaT_callTMres(L, tm, o, o, ra);
    return;
  }
#endif

  switch (ttypetag(o)) {
    case LUA_VVECTOR2: setfltvalue(s2v(ra), luaVec_length2(val_(o).f4)); break;
    case LUA_VVECTOR3: setfltvalue(s2v(ra), luaVec_length3(val_(o).f4)); break;
    case LUA_VVECTOR4: setfltvalue(s2v(ra), luaVec_length4(val_(o).f4)); break;
    case LUA_VQUAT: setfltvalue(s2v(ra), luaVec_length4(val_(o).f4)); break;
    default:
      luaG_runerror(L, "Invalid arguments, vector type required.");
      break;
  }
}

/* }================================================================== */

/*
** {==================================================================
** Object
** ===================================================================
*/

#define l_vfx(v) ((LUA_VEC_UACNUMBER)(v).x)
#define l_vfy(v) ((LUA_VEC_UACNUMBER)(v).y)
#define l_vfz(v) ((LUA_VEC_UACNUMBER)(v).z)
#define l_vfw(v) ((LUA_VEC_UACNUMBER)(v).w)

#if !defined(LUA_USE_C89)
  #define l_vprintf(s,sz,f,...) snprintf(s,sz,f,__VA_ARGS__)
#elif defined(LUA_USE_WINDOWS) && defined(__STDC_WANT_SECURE_LIB__)
  #define l_vprintf(s,sz,f,...) sprintf_s(s,sz,f,__VA_ARGS__)
#else
  #define l_vprintf(s,sz,f,...) ((void)(sz), sprintf(s,f,__VA_ARGS__))
#endif

int luaVec_rawget (lua_State *L, const lua_Float4 *v, int vdims, StkId key) {
  lua_Integer dim = 0;
  const TValue *k = s2v(key);
  if (ttisinteger(k) && (dim = ivalue(k)) >= 1 && dim <= vdims) {
    setfltvalue(s2v(L->top), cast_num(luaV_getf4(v, (int)(dim - 1))));  /* Accessing is 0-based */
    return LUA_TNUMBER;
  }
  else if (ttisstring(k)) {
    lua_Float4 out;
    switch (luaVec_swizzle(svalue(k), v, vdims, &out)) {
      case 1: setfltvalue(s2v(L->top), cast_num(out.x)); return LUA_TNUMBER;
      case 2: setvvalue(s2v(L->top), out, LUA_VVECTOR2); return LUA_TVECTOR;
      case 3: setvvalue(s2v(L->top), out, LUA_VVECTOR3); return LUA_TVECTOR;
      case 4: setvvalue(s2v(L->top), out, LUA_VVECTOR4); return LUA_TVECTOR;
      default:
        break;
    }
  }
  setnilvalue(s2v(L->top));
  return LUA_TNIL;
}

int luaVec_next (lua_State *L, const lua_Float4 *v, int vdims, StkId key) {
  int more = 0;
  const TValue *k = s2v(key);
  if (ttisnil(k)) {
    setsvalue(L, s2v(key), luaS_new(L, dims[0]));
    setfltvalue(s2v(key + 1), cast_num(v->x));
    more = 1;
  }
  else if (ttisinteger(k)) {
    lua_Integer dim = ivalue(k);
    if (dim >= 0 && dim < vdims) {  /* Allow 0 as an initial value (although invalid) */
      setivalue(s2v(key), dim + 1);  /* Iterator values are 1-based */
      setfltvalue(s2v(key + 1), cast_num(luaV_getf4(v, (int)dim)));  /* Accessing is 0-based */
      more = 1;
    }
  }
  else if (ttisstring(k)) {
    int dim = luaV_checkdimension(svalue(k));
    if (dim >= 0 && dim < (vdims - 1)) {
      setsvalue(L, s2v(key), luaS_new(L, dims[dim + 1]));
      setfltvalue(s2v(key + 1), cast_num(luaV_getf4(v, dim + 1)));
      more = 1;
    }
  }
  return more;
}

int luaVec_tostr (char *buff, size_t len, const lua_Float4 v, int variant) {
  if (len < LUAI_MAXVECTORSTR)
    return 0;

  switch (variant) {
    /* Taken from lobject.c: tostringbuff */
    case LUA_VNUMFLT: {
      int nb = lua_number2str(buff, len, v.x);
      if (buff[strspn(buff, "-0123456789")] == '\0') {  /* looks like an int? */
        buff[nb++] = lua_getlocaledecpoint();
        buff[nb++] = '0';  /* adds '.0' to result */
      }
      return nb;
    }
    case LUA_VVECTOR2:
      return l_vprintf(buff, len, "vector2("LUA_VEC_NUMBER_FMT", "
                                   ""LUA_VEC_NUMBER_FMT")", l_vfx(v), l_vfy(v));
    case LUA_VVECTOR3:
      return l_vprintf(buff, len, "vector3("LUA_VEC_NUMBER_FMT", "
                                  ""LUA_VEC_NUMBER_FMT", "LUA_VEC_NUMBER_FMT")",
                                                  l_vfx(v), l_vfy(v), l_vfz(v));
    case LUA_VVECTOR4:
      return l_vprintf(buff, len, "vector4("LUA_VEC_NUMBER_FMT", "
            ""LUA_VEC_NUMBER_FMT", "LUA_VEC_NUMBER_FMT", "LUA_VEC_NUMBER_FMT")",
                                        l_vfx(v), l_vfy(v), l_vfz(v), l_vfw(v));
    case LUA_VQUAT:
      return l_vprintf(buff, len, "quat("LUA_VEC_NUMBER_FMT", "
            ""LUA_VEC_NUMBER_FMT", "LUA_VEC_NUMBER_FMT", "LUA_VEC_NUMBER_FMT")",
                                        l_vfw(v), l_vfx(v), l_vfy(v), l_vfz(v));
    default:
      return 0;
  }
}

int luaVec_pullstring (lua_State *L, const TValue *o, lua_Float4 *sink) {
  if (ttisstring(o)) {
    sink->x = sink->y = sink->z = sink->w = V_ZERO;
    luaG_runerror(L, "luaVec_pullstring not yet implemented");
  }
  else {
    luaG_runerror(L, "invalid vectorstring option");
  }
}

LUA_API const char *lua_pushvecstring (lua_State *L, int idx) {
  if (lua_isinteger(L, idx))
    return lua_pushfstring(L, LUA_INTEGER_FMT, lua_tointeger(L, idx));
  else if (lua_isnumber(L, idx))
    return lua_pushfstring(L, LUA_NUMBER_FMT, (LUAI_UACNUMBER) lua_tonumber(L, idx));
  else if (lua_isvector(L, idx, V_NOTABLE)) {
    lua_Float4 v;
    int variant = lua_tovector(L, idx, V_NOTABLE, &v);
#if defined(GRIT_LONG_STRING)
    switch (variant) {
      case LUA_VNUMFLT: return lua_pushfstring(L, "%f", v.x);
      case LUA_VVECTOR2: return lua_pushfstring(L, "vector2(%f, %f)", v.x, v.y);
      case LUA_VVECTOR3: return lua_pushfstring(L, "vector3(%f, %f, %f)", v.x, v.y, v.z);
      case LUA_VVECTOR4: return lua_pushfstring(L, "vector4(%f, %f, %f, %f)", v.x, v.y, v.z, v.w);
      case LUA_VQUAT: return lua_pushfstring(L, "quat(%f, %f, %f, %f)", v.w, v.x, v.y, v.z);
      default:
        luaG_runerror(L, "invalid vectorstring option");
    }
#else
    char buff[LUAI_MAXVECTORSTR] = { 0 };
    if (luaVec_tostr(buff, LUAI_MAXVECTORSTR, v, variant) > 0)
      return lua_pushfstring(L, "%s", buff);
    else
      return NULL;
#endif
  }
  else
    luaG_runerror(L, "invalid vectorstring option");
}

/* }================================================================== */

/*
** {==================================================================
** LVM
** ===================================================================
*/

#define K_VECTOR_DIMENSIONS "dim"
#define K_QUATERNION_ANGLE "angle"
#define K_QUATERNION_AXIS "axis"

void luaVec_getstring (lua_State *L, const TValue* t, const char* skey, TValue* key, StkId val) {
  lua_Float4 out;
  const TValue *tm = luaT_gettmbyobj(L, t, TM_INDEX);  /* table's metamethod */
  switch (ttypetag(t)) {
    case LUA_VVECTOR3: {
      switch (luaVec_swizzle(skey, &val_(t).f4, 3, &out)) {
        case 1: setfltvalue(s2v(val), cast_num(out.x)); return;
        case 2: setvvalue(s2v(val), out, LUA_VVECTOR2); return;
        case 3: setvvalue(s2v(val), out, LUA_VVECTOR3); return;
        case 4: setvvalue(s2v(val), out, LUA_VVECTOR4); return;
        default: {
          if (ttisnil(tm) && strcmp(skey, K_VECTOR_DIMENSIONS) == 0) {
            setfltvalue(s2v(val), 3);
            return;
          }
          break;
        }
      }
      break;
    }
    case LUA_VQUAT: {
      switch (luaVec_swizzle(skey, &val_(t).f4, 4, &out)) {
        case 1: setfltvalue(s2v(val), cast_num(out.x)); return;
        case 2: setvvalue(s2v(val), out, LUA_VVECTOR2); return;
        case 3: setvvalue(s2v(val), out, LUA_VVECTOR3); return;
        case 4: setvvalue(s2v(val), out, LUA_VVECTOR4); return;
        default: {
          if (ttisnil(tm)) {  /* axis & angle can be overridden by a metatable */
            lua_Float4 v = val_(t).f4;
            if (strcmp(skey, K_QUATERNION_ANGLE) == 0) {
              setfltvalue(s2v(val), luaVec_axisangle(v));
              return;
            }
            else if (strcmp(skey, K_QUATERNION_AXIS) == 0) {
              lua_Float4 r;
              if (luaVec_axis(v, &r)) {
                setvvalue(s2v(val), r, LUA_VVECTOR3);
              }
              else
                luaG_runerror(L, "Identity quaternion has no axis");
              return;
            }
          }
          break;
        }
      }
      break;
    }
    case LUA_VVECTOR2: {
      switch (luaVec_swizzle(skey, &val_(t).f4, 2, &out)) {
        case 1: setfltvalue(s2v(val), cast_num(out.x)); return;
        case 2: setvvalue(s2v(val), out, LUA_VVECTOR2); return;
        case 3: setvvalue(s2v(val), out, LUA_VVECTOR3); return;
        case 4: setvvalue(s2v(val), out, LUA_VVECTOR4); return;
        default: {
          if (ttisnil(tm) && strcmp(skey, K_VECTOR_DIMENSIONS) == 0) {
            setfltvalue(s2v(val), 2);
            return;
          }
          break;
        }
      }
      break;
    }
    case LUA_VVECTOR4: {
      switch (luaVec_swizzle(skey, &val_(t).f4, 4, &out)) {
        case 1: setfltvalue(s2v(val), cast_num(out.x)); return;
        case 2: setvvalue(s2v(val), out, LUA_VVECTOR2); return;
        case 3: setvvalue(s2v(val), out, LUA_VVECTOR3); return;
        case 4: setvvalue(s2v(val), out, LUA_VVECTOR4); return;
        default: {
          if (ttisnil(tm) && strcmp(skey, K_VECTOR_DIMENSIONS) == 0) {
            setfltvalue(s2v(val), 4);
            return;
          }
          break;
        }
      }
      break;
    }
    default:
      break;
  }

  if (ttisnil(tm)) {
    luaG_runerror(L, "invalid vector field: '%s'", skey);
  } else {
    luaV_finishget(L, t, key, val, NULL);
  }
}

void luaVec_getint (lua_State *L, const TValue *t, const lua_Integer key, TValue* pkey, StkId val) {
  const TValue *tm;
  switch (ttypetag(t)) {
    case LUA_VVECTOR3: {
      switch(key) {
        case 1: setfltvalue(s2v(val), cast_num(val_(t).f4.x)); return;
        case 2: setfltvalue(s2v(val), cast_num(val_(t).f4.y)); return;
        case 3: setfltvalue(s2v(val), cast_num(val_(t).f4.z)); return;
        default: break;
      }
      break;
    }
    case LUA_VQUAT:
    case LUA_VVECTOR4: {
      switch(key) {
        case 1: setfltvalue(s2v(val), cast_num(val_(t).f4.x)); return;
        case 2: setfltvalue(s2v(val), cast_num(val_(t).f4.y)); return;
        case 3: setfltvalue(s2v(val), cast_num(val_(t).f4.z)); return;
        case 4: setfltvalue(s2v(val), cast_num(val_(t).f4.w)); return;
        default: break;
      }
      break;
    }
    case LUA_VVECTOR2: {
      switch(key) {
        case 1: setfltvalue(s2v(val), cast_num(val_(t).f4.x)); return;
        case 2: setfltvalue(s2v(val), cast_num(val_(t).f4.y)); return;
        default: break;
      }
      break;
    }
    default:
      break;
  }

  tm = luaT_gettmbyobj(L, t, TM_INDEX);  /* table's metamethod */
  if (ttisnil(tm))
    luaG_typeerror(L, t, "index");
  else
    luaV_finishget(L, t, pkey, val, NULL);
}

#if defined(GRIT_USE_PATH)
#include <stdlib.h>

/* This function is hot garbage */
TString *resolve_absolute_path (lua_State *L, const char *file, const char *rel) {
  size_t i;
  size_t file_len, rel_len;
  char *file2, *rel2;
  char **pieces, **pieces2;
  char *str;
  size_t piece_id = 0;
  size_t pieces2_id = 0;
  TString *r;

  file_len = strlen(file);
  rel_len = strlen(rel);

  if ((file2 = (char*)malloc(file_len + 1)) == NULL) {
    luaG_runerror(L, "allocation error");
  } else if ((rel2 = (char*)malloc(rel_len + 1)) == NULL) {
    free(file2);
    luaG_runerror(L, "allocation error");
  }

#if defined(LUA_USE_WINDOWS) && defined(__STDC_WANT_SECURE_LIB__)
  strcpy_s(file2, file_len + 1, file);
  strcpy_s(rel2, rel_len + 1, rel);
#else
  strcpy(file2, file);
  strcpy(rel2, rel);
#endif
  /*
  ** Conservative upper limit on the number of pieces.
  ** Even this is a tiny amount of memory though, so we're fine.
  */
  if ((pieces = (char **) malloc(sizeof(char*) * (file_len + rel_len))) == NULL) {
    free(file2); free(rel2);
    luaG_runerror(L, "allocation error");
  }
  else if ((pieces2 = (char **) malloc(sizeof(char*) * (file_len + rel_len))) == NULL) {
    free(file2); free(rel2); free(pieces);
    luaG_runerror(L, "allocation error");
  }

  if (rel2[0] != '/') {
    int has_slash = 0;
    size_t last_slash = 0;

    lua_assert(file2[0] == '/');
    /* Note: this strips everything after the last */
    for (i = 1; i < file_len; ++i) {
      if (file2[i] == '/') {
        file2[i] = '\0';
        pieces[piece_id++] = &file2[last_slash+1];
        last_slash = i;
      }
    }

    last_slash = 0;
    for (i = 0; i < rel_len; ++i) {
      if (rel2[i] == '/') {
        rel2[i] = '\0';
        pieces[piece_id++] = has_slash ? &rel2[last_slash+1] : &rel2[0];
        last_slash = i;
        has_slash = 1;
      }
    }
    pieces[piece_id++] = &rel2[last_slash+1];
  }
  else {
    size_t last_slash = 0;
    for (i = 1; i < rel_len; ++i) {
      if (rel2[i] == '/') {
        rel2[i] = '\0';
        pieces[piece_id++] = &rel2[last_slash+1];
        last_slash = i;
      }
    }
    pieces[piece_id++] = &rel2[last_slash+1];
  }

  /* compress out the . and .. */
  {
    size_t sz = 2; /* leading / and terminating '\0' */
    for (i=0 ; i<piece_id ; ++i) {
      if (!strcmp(pieces[i], ".")) {
        /* skip it */
      } else if (!strcmp(pieces[i], "")) {
        /* skip it */
      } else if (!strcmp(pieces[i], "..")) {
        /* condition ensures that /../foo is handled (by ignoring the ..) */
        if (pieces2_id == 0) {
          free(pieces2); free(pieces); free(file2); free(rel2);
          luaG_runerror(L, "Too many .. in path.");
        }
        pieces2_id--;
      } else {
        pieces2[pieces2_id++] = pieces[i];
        sz += strlen(pieces[i]) + 1; /* include next / or '\0' */
      }
    }
    if ((str = (char*)malloc(sz)) == NULL) {
      free(pieces2); free(pieces); free(file2); free(rel2);
      luaG_runerror(L, "allocation error");
    }
  }
  free(pieces);

  /* now concatenate into a string */
  {
    size_t str_id = 0;
    str[str_id++] = '/';
    for (i=0 ; i<pieces2_id ; ++i) {
      size_t j=0;
      char *piece;
      size_t piece_len;
      piece = pieces2[i];
      piece_len = strlen(piece);
      if (i > 0) str[str_id++] = '/';
      for (j=0 ; j<piece_len ; ++j)
        str[str_id++] = piece[j];
    }
    str[str_id++] = '\0';
  }
  free(pieces2);

  r = luaS_new(L, str);
  free(str);

  free(file2);
  free(rel2);
  return r;
}
#endif

/* }================================================================== */

