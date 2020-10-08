/*
** $Id: lgrit.c $
**
** Core & library lua vector functions. Note as this codebase develops, it will
** eventually need to be split.
**
** See Copyright Notice in lua.h
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
#if LUA_VVECTOR1 != LUA_VNUMFLT
  #error "Invalid implicit vector variant"
#endif

/* Dimension strings: c_dims + \0, must be consistent with luaVec_swizzle */
static const char* const dims[] = { "x", "y", "z", "w" };

/* If you lie to the compiler, it will get its revenge. */
static LUA_INLINE void luaV_assignf4 (lua_Float4 *to, int index, lua_VecF n) {
  switch (index) {
    case 0: to->x = n; break;
    case 1: to->y = n; break;
    case 2: to->z = n; break;
    case 3: to->w = n; break;
    default: break;
  }
}

static LUA_INLINE lua_VecF luaV_getf4 (const lua_Float4 *to, int index) {
  switch (index) {
    case 0: return to->x;
    case 1: return to->y;
    case 2: return to->z;
    case 3: return to->w;
    default:
      return V_ZERO;
  }
}

static int luaVec_swizzle (const char *key, size_t key_len,
                          const lua_Float4 *from, int from_sz, lua_Float4 *to) {
  int counter = 0;
  if (key_len == 0 || key_len > 4)
    return 0;

  for (; counter < (int)key_len; ++counter) {
    switch (key[counter]) {
      case 'x': if (from_sz < 1) return 0; luaV_assignf4(to, counter, from->x); break;
      case 'y': if (from_sz < 2) return 0; luaV_assignf4(to, counter, from->y); break;
      case 'z': if (from_sz < 3) return 0; luaV_assignf4(to, counter, from->z); break;
      case 'w': if (from_sz < 4) return 0; luaV_assignf4(to, counter, from->w); break;
      default: return 0;
    }
  }
  return counter;
}

/*
** {==================================================================
** Table API
** ===================================================================
*/

int luaVec_parse (lua_State* L, const TValue *o, lua_Float4 *v) {
  if (ttisvector(o)) {
    if (v != NULL) *v = vvalue(o);
    return luaVec_dimensions(o);
  }
  else if (ttistable(o)) {
    int count = 0;
    if (v != NULL)  /* Ensure vector data is cleared */
      v->x = v->y = v->z = v->w = V_ZERO;

    for (int i = 0; i < 4; ++i) {
      const TValue *slot = NULL;
      TString *str = luaS_new(L, dims[i]);
      if (luaV_fastget(L, o, str, slot, luaH_getstr) && ttisnumber(slot)) {
        count++;
        if (v != NULL)
          luaV_assignf4(v, i, cast_vec(nvalue(slot)));
      }
    }
    return count;
  }
  return 0;
}

/* }================================================================== */

/*
** {==================================================================
** Base
** ===================================================================
*/

static int luaB_vectorn (lua_State *L, int max_size, lua_Float4 *input) {
  int counter = 0, i;
  for (i = 1; i <= lua_gettop(L); ++i) {
    if (lua_isnumber(L, i)) {
      if (counter + 1 > max_size) return 0;
      luaV_assignf4(input, counter++, cast_vec(lua_tonumber(L, i)));
    }
    else if (lua_isvector(L, i, V_PARSETABLE)) {
      lua_Float4 f4;
      switch (lua_tovector(L, i, V_PARSETABLE, &f4)) {
        case LUA_VVECTOR1:
          if (counter + 1 > max_size) return 0;
          luaV_assignf4(input, counter++, f4.x);
          break;
        case LUA_VVECTOR2:
          if (counter + 2 > max_size) return 0;
          luaV_assignf4(input, counter++, f4.x);
          luaV_assignf4(input, counter++, f4.y);
          break;
        case LUA_VVECTOR3:
          if (counter + 3 > max_size) return 0;
          luaV_assignf4(input, counter++, f4.x);
          luaV_assignf4(input, counter++, f4.y);
          luaV_assignf4(input, counter++, f4.z);
          break;
        case LUA_VVECTOR4:
        case LUA_VQUAT:
          if (counter + 4 > max_size) return 0;
          luaV_assignf4(input, counter++, f4.x);
          luaV_assignf4(input, counter++, f4.y);
          luaV_assignf4(input, counter++, f4.z);
          luaV_assignf4(input, counter++, f4.w);
          break;
        default:
          return luaL_error(L, "unexpected " LABEL_VECTOR " type");
      }
    }
    else {
      return luaL_error(L, LABEL_VECTOR "%d(...) argument %d had type %s", max_size, i, lua_typename(L, lua_type(L, i)));
    }
  }
  return counter;
}

LUA_API const char *lua_dimension_label (lua_State *L, int idx) {
  UNUSED(L);
  return (idx >= 1 && idx <= 4) ? dims[idx - 1] : NULL;
}

LUA_API int lua_dimensions_count (lua_State *L, int tp) {
  switch (tp) {
    case LUA_VVECTOR1: return 1;
    case LUA_VVECTOR2: return 2;
    case LUA_VVECTOR3: return 3;
    case LUA_VVECTOR4: return 4;
    case LUA_VQUAT: return 4;
    default:
      return luaL_typeerror(L, tp, "vectortype");
  }
}

LUA_API const char *lua_vectypename (lua_State *L, int t) {
  switch (t) {
    case LUA_VVECTOR1: return LABEL_VECTOR1;
    case LUA_VVECTOR2: return LABEL_VECTOR2;
    case LUA_VVECTOR3: return LABEL_VECTOR3;
    case LUA_VVECTOR4: return LABEL_VECTOR4;
    case LUA_VQUAT: return LABEL_QUATERN;
    default:
      luaL_typeerror(L, t, "vectortype");
      return "";
  }
}

int lua_vectorN (lua_State *L) {
  lua_Float4 v = V_ZEROVEC;
  switch (luaB_vectorn(L, 4, &v)) {
    case 4: lua_pushvector(L, v, LUA_VVECTOR4); break;
    case 3: lua_pushvector(L, v, LUA_VVECTOR3); break;
    case 2: lua_pushvector(L, v, LUA_VVECTOR2); break;
    case 1: lua_pushnumber(L, (lua_Number)v.x); break;
    default:
      return luaL_error(L, "vec(...) takes 1 to 4 number arguments");
  }
  return 1;
}

int lua_vector2 (lua_State *L) {
  lua_Float4 v;
  if (luaB_vectorn(L, 2, &v) == 2) {
    lua_pushvector(L, v, LUA_VVECTOR2);
    return 1;
  }
  return luaL_error(L, LABEL_VECTOR2 "(...) requires exactly 2 numbers");
}

int lua_vector3 (lua_State *L) {
  lua_Float4 v;
  if (luaB_vectorn(L, 3, &v) == 3) {
    lua_pushvector(L, v, LUA_VVECTOR3);
    return 1;
  }
  return luaL_error(L, LABEL_VECTOR3 "(...) requires exactly 3 numbers");
}

int lua_vector4 (lua_State *L) {
  lua_Float4 v;
  if (luaB_vectorn(L, 4, &v) == 4) {
    lua_pushvector(L, v, LUA_VVECTOR4);
    return 1;
  }
  return luaL_error(L, LABEL_VECTOR4"(...) requires exactly 4 numbers");
}

int lua_quat (lua_State *L) {
  lua_Float4 q = { .x = V_ZERO, .y = V_ZERO, .z = V_ZERO, .w = V_ONE };
  if (lua_gettop(L) == 4 && lua_isnumber(L, 1) && lua_isnumber(L, 2)
                         && lua_isnumber(L, 3) && lua_isnumber(L, 4)) {
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
    return luaL_error(L, "Invalid params, try " LABEL_QUATERN "(n,n,n,n) " \
                               LABEL_QUATERN "(n,v3) " LABEL_QUATERN "(v3,v3)");
  }

  lua_pushvector(L, q, LUA_VQUAT);
  return 1;
}

int lua_unpackvec (lua_State *L) {
  luaL_checkstack(L, 4, "vector fields");  /* Ensure stack-space */
  if (lua_isinteger(L, 1))
    lua_pushinteger(L, luaL_checkinteger(L, 1));
  else if (lua_isnumber(L, 1))
    lua_pushnumber(L, luaL_checknumber(L, 1));
  else {
    lua_Float4 v;
    switch (lua_tovector(L, 1, V_NOTABLE, &v)) {
      case LUA_VVECTOR1:
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
        return luaL_error(L, "vunpack takes a " LABEL_NUMBER ", " LABEL_VECTOR2 ", " \
                        LABEL_VECTOR3 ", " LABEL_VECTOR4 ", or " LABEL_QUATERN);
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
    case LUA_VVECTOR1: setfltvalue(s2v(ra), val_(o).n); break;
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

int luaVec_rawget (lua_State *L, const lua_Float4 *v, int vdims, TValue *key) {
  lua_Integer dim = 0;
  UNUSED(L);
  if (ttisinteger(key) && (dim = ivalue(key)) >= 1 && dim <= vdims) {
    setfltvalue(key, cast_num(luaV_getf4(v, (int)(dim - 1))));  /* Accessing is 0-based */
    return LUA_TNUMBER;
  }
  else if (ttisstring(key)) {
    lua_Float4 out;
    switch (luaVec_swizzle(svalue(key), vslen(key), v, vdims, &out)) {
      case 1: setfltvalue(key, cast_num(out.x)); return LUA_TNUMBER;
      case 2: setvvalue(key, out, LUA_VVECTOR2); return LUA_TVECTOR;
      case 3: setvvalue(key, out, LUA_VVECTOR3); return LUA_TVECTOR;
      case 4: setvvalue(key, out, LUA_VVECTOR4); return LUA_TVECTOR;
      default:
        break;
    }
  }
  setnilvalue(key);
  return LUA_TNIL;
}

int luaVec_rawgeti (lua_State* L, const lua_Float4* v, int vdims, lua_Integer n) {
  if (n >= 1 && n <= vdims) {  /* Accessing is 0-based */
    setfltvalue(s2v(L->top), cast_num(luaV_getf4(v, (int)(n - 1))));
    return LUA_TNUMBER;
  }
  setnilvalue(s2v(L->top));
  return LUA_TNIL;
}

int luavec_getstr (lua_State *L, const lua_Float4 *v, int vdims, const char *k) {
  lua_Float4 out;
  switch (luaVec_swizzle(k, strlen(k), v, vdims, &out)) {
    case 1: setfltvalue(s2v(L->top), cast_num(out.x)); return LUA_TNUMBER;
    case 2: setvvalue(s2v(L->top), out, LUA_VVECTOR2); return LUA_TVECTOR;
    case 3: setvvalue(s2v(L->top), out, LUA_VVECTOR3); return LUA_TVECTOR;
    case 4: setvvalue(s2v(L->top), out, LUA_VVECTOR4); return LUA_TVECTOR;
    default:
      setnilvalue(s2v(L->top));
      return LUA_TNIL;
  }
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
    const lua_Integer dim = ivalue(k);
    if (dim >= 0 && dim < vdims) {  /* Allow 0 as an initial value (although invalid) */
      setivalue(s2v(key), dim + 1);  /* Iterator values are 1-based */
      setfltvalue(s2v(key + 1), cast_num(luaV_getf4(v, (int)dim)));  /* Accessing is 0-based */
      more = 1;
    }
  }
  else if (ttisstring(k) && vslen(k) == 1) {
    int dim = 0;
    switch (svalue(k)[0]) {
      case 'x': dim = 0; break;
      case 'y': dim = 1; break;
      case 'z': dim = 2; break;
      case 'w': dim = 3; break;
      default:
        return more;
    }

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
    case LUA_VVECTOR1: {
      int nb = lua_number2str(buff, len, v.x);
      if (buff[strspn(buff, "-0123456789")] == '\0') {  /* looks like an int? */
        buff[nb++] = lua_getlocaledecpoint();
        buff[nb++] = '0';  /* adds '.0' to result */
      }
      return nb;
    }
    case LUA_VVECTOR2:
      return l_vprintf(buff, len, "" LABEL_VECTOR2 "(" LUA_VEC_FMT ", " LUA_VEC_FMT ")", l_vfx(v), l_vfy(v));
    case LUA_VVECTOR3:
      return l_vprintf(buff, len, "" LABEL_VECTOR3 "(" LUA_VEC_FMT ", " LUA_VEC_FMT ", " LUA_VEC_FMT ")", l_vfx(v), l_vfy(v), l_vfz(v));
    case LUA_VVECTOR4:
      return l_vprintf(buff, len, "" LABEL_VECTOR4 "(" LUA_VEC_FMT ", " LUA_VEC_FMT ", " LUA_VEC_FMT ", " LUA_VEC_FMT ")", l_vfx(v), l_vfy(v), l_vfz(v), l_vfw(v));
    case LUA_VQUAT:
      return l_vprintf(buff, len, "" LABEL_QUATERN "(" LUA_VEC_FMT ", " LUA_VEC_FMT ", " LUA_VEC_FMT ", " LUA_VEC_FMT ")", l_vfw(v), l_vfx(v), l_vfy(v), l_vfz(v));
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
  else if (lua_isvector(L, idx, V_PARSETABLE)) {
    lua_Float4 v;
    int variant = lua_tovector(L, idx, V_PARSETABLE, &v);
    char buff[LUAI_MAXVECTORSTR] = { 0 };
    if (luaVec_tostr(buff, LUAI_MAXVECTORSTR, v, variant) > 0)
      return lua_pushfstring(L, "%s", buff);
    else
      return NULL;
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

void luaVec_getstring (lua_State *L, const TValue *t, const char *skey, TValue *key, StkId val) {
  int out_s;
  lua_Float4 out;
  const TValue *tm;
  if (ttisnumber(t))  /* Ignore swizzling numbers/implicit vec1. */
    luaV_finishget(L, t, key, val, NULL);
  else if ((out_s = luaVec_swizzle(skey, vslen(key), &val_(t).f4, luaVec_dimensions(t), &out)) > 0) {
    switch (out_s) {
      case 1: setfltvalue(s2v(val), cast_num(out.x)); break;
      case 2: setvvalue(s2v(val), out, LUA_VVECTOR2); break;
      case 3: setvvalue(s2v(val), out, LUA_VVECTOR3); break;
      case 4: setvvalue(s2v(val), out, LUA_VVECTOR4); break;
      default:
        setfltvalue(s2v(val), cast_num(0));
        break;
    }
  }
  /* Dimension fields takes priority over metamethods */
  else if (strcmp(skey, "dim") == 0 || strcmp(skey, "n") == 0) {
    setivalue(s2v(val), luaVec_dimensions(t));
  }
  /* Default functions when the LUA_TVECTOR does not have a debug metatable. */
  else if (tm = luaT_gettmbyobj(L, t, TM_INDEX), ttisnil(tm)) {
    if (ttisquat(t)) {
      if (strcmp(skey, "angle") == 0) {
        setfltvalue(s2v(val), luaVec_axisangle(val_(t).f4));
      }
      else if (strcmp(skey, "axis") == 0) {
        lua_Float4 r;
        luaVec_axis(val_(t).f4, &r);
        setvvalue(s2v(val), r, LUA_VVECTOR3);
      }
      else
        luaG_runerror(L, "invalid " LABEL_QUATERN " field: '%s'", skey);
    }
    else
      luaG_runerror(L, "invalid " LABEL_VECTOR " field: '%s'", skey);
  }
  else {
    luaV_finishget(L, t, key, val, NULL);
  }
}

void luaVec_getint (lua_State *L, const TValue *t, const lua_Integer key, TValue *pkey, StkId val) {
  const TValue *tm;
  const int dimensions = luaVec_dimensions(t);
  if (dimensions > 1 && key >= 1 && key <= dimensions) {
    switch(key) {
      case 1: setfltvalue(s2v(val), cast_num(val_(t).f4.x)); return;
      case 2: setfltvalue(s2v(val), cast_num(val_(t).f4.y)); return;
      case 3: setfltvalue(s2v(val), cast_num(val_(t).f4.z)); return;
      case 4: setfltvalue(s2v(val), cast_num(val_(t).f4.w)); return;
      default:
        break;
    }
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

