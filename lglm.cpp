/*
** $Id: lglm.cpp $
** Vector/Matirx object definitions.
** See Copyright Notice in lua.h
*/

#define lglm_cpp
#define LUA_CORE

/*
@@ LUA_C_LINKAGE is a marker for linking objects against a Lua runtime built
**  with a different linkage specification, e.g., linking C++ objects against
**  a runtime compiled with C.
@@ LUA_API_LINKAGE is a mark for the linkage specification to core API functions
*/
#if defined(LUA_C_LINKAGE)
  #define LUA_API_LINKAGE "C"
#else
  #define LUA_API_LINKAGE "C++"
#endif

extern LUA_API_LINKAGE {
  #include "luaconf.h"
  #include "lua.h"
  #include "ldebug.h"
  #include "lfunc.h"
  #include "lgc.h"
  #include "lstate.h"
  #include "lstring.h"
  #include "ltable.h"
  #include "lvm.h"

  #include "lapi.h"
  #include "lauxlib.h"
  #include "lgrit_lib.h"
  #include "lglm_core.h"
}

#include "lglm.hpp"
#include "lglm_string.hpp"

#include <string>

#include <glm/glm.hpp>
#include <glm/fwd.hpp>
#include <glm/ext.hpp>

#include <glm/detail/type_quat.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/common.hpp>
#include <glm/gtx/exterior_product.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/ext/matrix_relational.hpp>
#include <glm/ext/matrix_transform.hpp>

#define INVALID_VECTOR_TYPE ("invalid " LABEL_VECTOR " type")
#define INVALID_VECTOR_STRUCTURE ("invalid " LABEL_VECTOR " structure")
#define INVALID_VECTOR_DIMENSIONS ("invalid " LABEL_VECTOR " dimension")
#define INVALID_MATRIX_STRUCTURE ("invalid " LABEL_MATRIX " structure")
#define INVALID_MATRIX_DIMENSIONS ("invalid " LABEL_MATRIX " dimension")

/* return helpers */
#define glm_runerror(L, M) (luaG_runerror((L), (M)), 0)
#define glm_typeError(L, O, M) (luaG_typeerror((L), (O), (M)), 0)
#define glm_finishset(L, T, K, V) (luaV_finishset((L), (T), (K), (V), GLM_NULLPTR), 1)

/* convert an object to an integer (without string coercion) */
#define glm_tointeger(o) (ttisinteger(o) ? ivalue(o) : glm_flttointeger(o))

#define _gettop(L) (cast_int((L)->top - ((L)->ci->func + 1)))
#define _isvalid(L, o) (!ttisnil(o) || o != &G(L)->nilvalue)
#define _ispseudo(i) ((i) <= LUA_REGISTRYINDEX)

/* index2value ported from lapi.c */
static TValue *glm_index2value(lua_State *L, int idx) {
  CallInfo *ci = L->ci;
  if (idx > 0) {
    StkId o = ci->func + idx;
    api_check(L, idx <= L->ci->top - (ci->func + 1), "unacceptable index");
    if (o >= L->top)
      return &G(L)->nilvalue;
    else
      return s2v(o);
  }
  else if (!_ispseudo(idx)) { /* negative index */
    api_check(L, idx != 0 && -idx <= L->top - (ci->func + 1), "invalid index");
    return s2v(L->top + idx);
  }
  else if (idx == LUA_REGISTRYINDEX)
    return &G(L)->l_registry;
  else { /* upvalues */
    idx = LUA_REGISTRYINDEX - idx;
    api_check(L, idx <= MAXUPVAL + 1, "upvalue index too large");
    if (ttislcf(s2v(ci->func)))  /* light C function? */
      return &G(L)->nilvalue;  /* it has no upvalues */
    else {
      CClosure *func = clCvalue(s2v(ci->func));
      return (idx <= func->nupvalues) ? &func->upvalue[idx - 1] : &G(L)->nilvalue;
    }
  }
}

/* Parse the given tagged value as vector/matrix accessible index. */
static LUA_INLINE lua_Integer glm_flttointeger(const TValue *obj) {
  const lua_Number n = l_floor(fltvalue(obj));
  if (n >= cast_num(LUA_MININTEGER) && n < -cast_num(LUA_MININTEGER))
    return static_cast<lua_Integer>(n);
  return 0;
}

/* trybinTM where the first element is a number. */
static int num_trybinTM(lua_State *L, const TValue *p1, const TValue *p2, StkId res, TMS event);

/* trybinTM where the first element is a vector. */
static int vec_trybinTM(lua_State *L, const TValue *p1, const TValue *p2, StkId res, TMS event);

/* trybinTM where the first element is a quaternion. */
static int quat_trybinTM(lua_State *L, const TValue *p1, const TValue *p2, StkId res, TMS event);

/* trybinTM where the first element is a matrix. */
static int mat_trybinTM(lua_State *L, const TValue *p1, const TValue *p2, StkId res, TMS event);

/*
** {==================================================================
** Object Conversion
** ===================================================================
*/

/* raw object fields */
#define glm_vvalue_raw(o) glm_constvec_boundary(&vvalue_raw(o))

/* Future-proof for when/if quaternions have their own type tag */
#define glm_vvalue(o) glm_constvec_boundary(vvalue_ref(o))
#define glm_vecvalue(o) check_exp(ttisvector(o), glm_constvec_boundary(&vvalue_(o)))
#define glm_quatvalue(o) check_exp(ttisquat(o), glm_constvec_boundary(&vvalue_(o)))
#define glm_setvvalue2s(s, x, o)        \
  LUA_MLM_BEGIN                         \
  TValue *io = s2v(s);                  \
  glm_vec_boundary(&vvalue_(io)) = (x); \
  settt_(io, (o));                      \
  LUA_MLM_END

#define glm_mvalue(o) glm_constmat_boundary(mvalue_ref(o))
#define glm_setmvalue2s(L, o, x) glm_setmvalue(L, s2v(o), x)
#define glm_setmvalue(L, obj, x) \
  LUA_MLM_BEGIN                  \
  TValue *io = (obj);            \
  GCMatrix *x_ = (x);            \
  val_(io).gc = obj2gco(x_);     \
  settt_(io, ctb(LUA_VMATRIX));  \
  checkliveness(L, io);          \
  LUA_MLM_END

/* }================================================================== */

/*
** {==================================================================
** Vector Object API
** ===================================================================
*/

/* Finish the vector access */
static void vec_finishget(lua_State *L, const TValue *obj, TValue *key, StkId res) {
  const TValue *tm = luaT_gettmbyobj(L, obj, TM_INDEX);
  if (l_unlikely(notm(tm))) {  // gritLua compatibility: default meta-methods
    if (!ttisstring(key))
      luaG_typeerror(L, obj, "index");
    else if (ttisquat(obj)) {
      if (strcmp(svalue(key), "angle") == 0) {  // angle in degrees; gritLua compatibility.
        setfltvalue(s2v(res), glm::degrees(cast_num(glm::angle(glm_quatvalue(obj).q))));
      }
      else if (strcmp(svalue(key), "axis") == 0) {
        glmVector out(glm::axis(glm_quatvalue(obj).q));
        glm_setvvalue2s(res, out, LUA_VVECTOR3);
      }
      else {
#if defined(GLM_LUA_APICHECK)
        luaG_runerror(L, "invalid " LABEL_QUATERN " field: '%s'", svalue(key));
#else
        setnilvalue(s2v(res));
#endif
      }
    }
    else {
#if defined(GLM_LUA_APICHECK)
      luaG_runerror(L, "invalid " LABEL_VECTOR " field: '%s'", svalue(key));
#else
      setnilvalue(s2v(res));
#endif
    }
  }
  else {  // else will try the metamethod
    luaV_finishget(L, obj, key, res, NULL);
  }
}

/// <summary>
/// Runtime swizzle operation
/// </summary>
/// <param name="key">A null-terminated string.</param>
/// <returns>The number of copied vector elements; zero on failure.</returns>
template<glm::length_t L, typename T, glm::qualifier Q, glm::length_t LOut>
static glm::length_t swizzle(const glm::vec<L, T, Q> &v, const char *key, glm::vec<LOut, T, Q> &out) {
  glm::length_t counter = 0;
  if (key == GLM_NULLPTR)
    return counter;

  while (key[counter] != '\0' && counter <= LOut) {
    switch (key[counter]) {
      case 'x': GLM_IF_CONSTEXPR (L < 1) return 0; out[counter++] = static_cast<T>(v[0]); break;
      case 'y': GLM_IF_CONSTEXPR (L < 2) return 0; out[counter++] = static_cast<T>(v[1]); break;
      case 'z': GLM_IF_CONSTEXPR (L < 3) return 0; out[counter++] = static_cast<T>(v[2]); break;
      case 'w': GLM_IF_CONSTEXPR (L < 4) return 0; out[counter++] = static_cast<T>(v[3]); break;
      default:
        return 0;
    }
  }
  return counter;
}

template<typename T, glm::qualifier Q, glm::length_t LOut>
static glm::length_t swizzle(const glm::qua<T, Q> &q, const char *key, glm::vec<LOut, T, Q> &out) {
#if defined(GLM_FORCE_QUAT_DATA_WXYZ)
  const glm::vec<4, T, Q> v(q.w, q.x, q.y, q.z);
#else
  const glm::vec<4, T, Q> v(q.x, q.y, q.z, q.w);
#endif
  return swizzle(v, key, out);
}

int glmVec_rawgeti(const TValue *obj, lua_Integer n, StkId res) {
  if (vecgeti(obj, n, res) == LUA_TNONE)
    setnilvalue(s2v(res));
  return ttypetag(s2v(res));
}

int glmVec_rawgets(const TValue *obj, const char *k, StkId res) {
  const int result = strlen(k) == 1 ? vecgets(obj, k, res) : LUA_TNONE;
  if (result == LUA_TNONE)
    setnilvalue(s2v(res));
  return ttypetag(s2v(res));
}

int glmVec_rawget(const TValue *obj, TValue *key, StkId res) {
  int result = LUA_TNONE;
  switch (ttype(key)) {
    case LUA_TNUMBER: {
      result = vecgeti(obj, glm_tointeger(key), res);
      break;
    }
    case LUA_TSTRING: {
      if (vslen(key) == 1)
        result = vecgets(obj, svalue(key), res);
      break;
    }
    default:
      break;
  }

  if (result == LUA_TNONE)
    setnilvalue(s2v(res));
  return ttypetag(s2v(res));
}

void glmVec_geti(lua_State *L, const TValue *obj, lua_Integer c, StkId res) {
  if (vecgeti(obj, c, res) == LUA_TNONE) {  // Attempt metatable access
    TValue key;
    setivalue(&key, c);
    luaV_finishget(L, obj, &key, res, NULL);
  }
}

void glmVec_get(lua_State *L, const TValue *obj, TValue *key, StkId res) {
  if (ttisnumber(key)) {
    if (vecgeti(obj, glm_tointeger(key), res) != LUA_TNONE)
      return;
  }
  else if (ttisstring(key)) {
    const char* str = svalue(key);
    if (vslen(key) == 1) {  // hot-path single character access
      if (vecgets(obj, str, res) != LUA_TNONE)
        return;
    }
    else {
      const glmVector &v = glm_vvalue(obj);

      glmVector out;  // Allow runtime swizzle operations prior to metamethod access.
      const int count = ttisquat(obj) ? swizzle(v.q, str, out.v4) : swizzle(v.v4, str, out.v4);
      switch (count) {
        case 1: setfltvalue(s2v(res), cast_num(out.v4.x)); return;
        case 2: glm_setvvalue2s(res, out, LUA_VVECTOR2); return;
        case 3: glm_setvvalue2s(res, out, LUA_VVECTOR3); return;
        case 4: {
          // A swizzle quaternion that still results in a normalized vector4 remains a quaternion.
          if (ttisquat(obj) && glm::isNormalized(out.v4, glm::epsilon<glm_Float>()))
            glm_setvvalue2s(res, glm::qua<glm_Float>(out.v4.w, out.v4.x, out.v4.y, out.v4.z), LUA_VQUAT);
          else
            glm_setvvalue2s(res, out, LUA_VVECTOR4);
          return;
        }
        default: {  // gritLua compatibility: dimension field takes priority over metamethods
          if (strcmp(str, "dim") == 0) {
            setivalue(s2v(res), i_luaint(glm_dimensions(ttypetag(obj))));
            return;
          }
          break;
        }
      }
    }
  }
  vec_finishget(L, obj, key, res);  // Metatable Access
}

void glmVec_objlen(const TValue *obj, StkId res) {
  const glmVector &v = glm_vvalue(obj);
  switch (ttypetag(obj)) {
    case LUA_VVECTOR2: setfltvalue(s2v(res), cast_num(glm::length(v.v2))); break;
    case LUA_VVECTOR3: setfltvalue(s2v(res), cast_num(glm::length(v.v3))); break;
    case LUA_VVECTOR4: setfltvalue(s2v(res), cast_num(glm::length(v.v4))); break;
    case LUA_VQUAT: setfltvalue(s2v(res), cast_num(glm::length(v.q))); break;
    default:
      setfltvalue(s2v(res), cast_num(0));
      break;
  }
}

int glmVec_equalObj(lua_State *L, const TValue *o1, const TValue *o2, int rtt) {
  bool result = false;
  const glmVector &v = glm_vvalue(o1);
  const glmVector &other_v = glm_vvalue(o2);
  switch (rtt) {
    case LUA_VVECTOR2: result = _glmeq(v.v2, other_v.v2); break;
    case LUA_VVECTOR3: result = _glmeq(v.v3, other_v.v3); break;
    case LUA_VVECTOR4: result = _glmeq(v.v4, other_v.v4); break;
    case LUA_VQUAT: result = _glmeq(v.q, other_v.q); break;
    default:
      break;
  }

  if (result == false && L != GLM_NULLPTR) {
    const TValue *tm = luaT_gettmbyobj(L, o1, TM_EQ);
    if (!notm(tm)) {
      luaT_callTMres(L, tm, o1, o2, L->top);  /* call TM */
      result = !l_isfalse(s2v(L->top));
    }
  }
  return result;
}

int glmVec_concat(const TValue *obj, const TValue *value, StkId res) {
  const glmVector &v = glm_vvalue(obj);

  glmVector result = v;  // Copy
  glm::length_t dims = glm_dimensions(ttypetag(obj));  // Current dimensionality
  if (ttisinteger(value) && dims < 4)
    result.v4[dims++] = cast_glmfloat(ivalue(value));
  else if (ttisfloat(value) && dims < 4)
    result.v4[dims++] = cast_glmfloat(fltvalue(value));
  else if (ttisboolean(value) && dims < 4)
    result.v4[dims++] = cast_glmfloat(!l_isfalse(value));
  else if (ttisvector(value)) {
    const glm::length_t v_dims = glm_dimensions(ttypetag(value));
    if ((dims + v_dims) > 4)
      return 0;  // Outside valid dimensionality

    for (glm::length_t i = 0; i < v_dims; ++i)
      result.v4[dims++] = glm_vecvalue(value).v4[i];
  }
  else {
    return 0;
  }

  glm_setvvalue2s(res, result, glm_variant(dims));
  return 1;
}

int glmVec_tostr(const TValue *obj, char *buff, size_t len) {
  int copy = 0;
  const glmVector &v = glm_vvalue(obj);
  switch (ttypetag(obj)) {
    case LUA_VVECTOR1: copy = glm::detail::format_type(buff, len, v.v1); break;
    case LUA_VVECTOR2: copy = glm::detail::format_type(buff, len, v.v2); break;
    case LUA_VVECTOR3: copy = glm::detail::format_type(buff, len, v.v3); break;
    case LUA_VVECTOR4: copy = glm::detail::format_type(buff, len, v.v4); break;
    case LUA_VQUAT: copy = glm::detail::format_type(buff, len, v.q); break;
    default:
      break;
  }
  lua_assert(copy >= 0);
  return copy;
}

int glmVec_equalKey(const TValue *k1, const Node *n2, int rtt) {
  // @NOTE: Ideally glmcVec_hash would be used. However, that would put the
  // table in an invalid state: mainposition != equalkey.
  switch (withvariant(rtt)) {
    case LUA_VVECTOR2: return glm_vecvalue(k1).v2 == glm_vvalue_raw(keyval(n2)).v2;
    case LUA_VVECTOR3: return glm_vecvalue(k1).v3 == glm_vvalue_raw(keyval(n2)).v3;
    case LUA_VVECTOR4: return glm_vecvalue(k1).v4 == glm_vvalue_raw(keyval(n2)).v4;
    case LUA_VQUAT: return glm_quatvalue(k1).q == glm_vvalue_raw(keyval(n2)).q;
    default:
      return 0;
  }
}

size_t glmcVec_hash(const Value *kvl, int ktt) {
  switch (withvariant(ktt)) {
    case LUA_VVECTOR2: return glm::hash::hash(glm_vvalue_raw(*kvl).v2);
    case LUA_VVECTOR3: return glm::hash::hash(glm_vvalue_raw(*kvl).v3);
    case LUA_VVECTOR4: return glm::hash::hash(glm_vvalue_raw(*kvl).v4);
    case LUA_VQUAT: return glm::hash::hash(glm_vvalue_raw(*kvl).q);
    default:
      return 0xDEAD;  // C0D3
  }
}

int glmVec_next(const TValue *obj, StkId key) {
  TValue *key_obj = s2v(key);
  if (ttisnil(key_obj)) {
    setivalue(key_obj, 1);
    if (vecgeti(obj, 1, key + 1) == LUA_TNONE)
      setnilvalue(s2v(key + 1));
    return 1;
  }
  else if (ttisnumber(key_obj)) {
    const lua_Integer l_nextIdx = glm_tointeger(key_obj) + 1;
    const glm::length_t nextIdx = i_glmlen(l_nextIdx);
    if (nextIdx >= 1 && nextIdx <= glm_dimensions(ttypetag(obj))) {
      setivalue(key_obj, l_nextIdx);  // Iterator values are 1-based
      if (vecgeti(obj, l_nextIdx, key + 1) == LUA_TNONE)
        setnilvalue(s2v(key + 1));
      return 1;
    }
  }
  return 0;
}

int glm_trybinTM(lua_State *L, const TValue *p1, const TValue *p2, StkId res, TMS event) {
  switch (ttype(p1)) {
    case LUA_TNUMBER: return num_trybinTM(L, p1, p2, res, event);
    case LUA_TMATRIX: return mat_trybinTM(L, p1, p2, res, event);
    case LUA_TVECTOR: {
      if (ttisquat(p1))  // quaternion-specific implementation
        return quat_trybinTM(L, p1, p2, res, event);
      return vec_trybinTM(L, p1, p2, res, event);
    }
    default:
      break;
  }
  return 0;
}

LUA_API int glm_unpack_vector(lua_State *L, int idx) {
  luaL_checkstack(L, 4, "vector fields"); /* Ensure stack-space */
  const TValue *o = glm_index2value(L, idx);
  switch (ttypetag(o)) {
    case LUA_VVECTOR2:
      lua_pushnumber(L, cast_num(vecvalue(o).x));
      lua_pushnumber(L, cast_num(vecvalue(o).y));
      return 2;
    case LUA_VVECTOR3:
      lua_pushnumber(L, cast_num(vecvalue(o).x));
      lua_pushnumber(L, cast_num(vecvalue(o).y));
      lua_pushnumber(L, cast_num(vecvalue(o).z));
      return 3;
    case LUA_VVECTOR4:
      lua_pushnumber(L, cast_num(vecvalue(o).x));
      lua_pushnumber(L, cast_num(vecvalue(o).y));
      lua_pushnumber(L, cast_num(vecvalue(o).z));
      lua_pushnumber(L, cast_num(vecvalue(o).w));
      return 4;
    case LUA_VQUAT:
      lua_pushnumber(L, cast_num(glm_quatvalue(o).q.w));
      lua_pushnumber(L, cast_num(glm_quatvalue(o).q.x));
      lua_pushnumber(L, cast_num(glm_quatvalue(o).q.y));
      lua_pushnumber(L, cast_num(glm_quatvalue(o).q.z));
      return 4;
    default: {
      lua_pushvalue(L, idx);
      return 1;
    }
  }
}

/* }================================================================== */

/*
** {==================================================================
** Matrix Object API
** ===================================================================
*/

/*
** @PARAM raw: If true, the function will throw runtime errors to handle setting
**    invalid keys/fields in the matrix (i.e., the limitation on table semantics)
*/
static int glmMat_auxset(lua_State *L, const TValue *obj, TValue *key, TValue *val, bool raw) {
  if (!ttisnumber(key)) {  // Invalid index for matrix
    return raw ? glm_typeError(L, key, "index")
               : glm_finishset(L, obj, key, val);
  }

  glmMatrix &m = glm_mat_boundary(mvalue_ref(obj));
  const glm::length_t dim = i_glmlen(glm_tointeger(key));
  if (ttisvector(val)) {
    const bool expanding = dim <= 4 && (dim == (m.size + 1));
    if (glm_dimensions(ttypetag(val)) != m.secondary)  // Invalid vector being appended
      return raw ? glm_runerror(L, INVALID_MATRIX_DIMENSIONS) : glm_finishset(L, obj, key, val);
    else if (dim <= 0 || (dim > m.size && !expanding))  // Index out of bounds.
      return raw ? glm_runerror(L, INVALID_MATRIX_DIMENSIONS) : glm_finishset(L, obj, key, val);

    switch (m.secondary) {
      case 2: m.m42[dim - 1] = glm_vecvalue(val).v2; break;
      case 3: m.m43[dim - 1] = glm_vecvalue(val).v3; break;
      case 4: {
#if defined(GLM_FORCE_QUAT_DATA_WXYZ)  // Support GLM_FORCE_QUAT_DATA_WXYZ
        if (ttisquat(val)) {
          const glm::qua<glm_Float> &q = glm_quatvalue(val).q;
          m.m44[dim - 1] = glm::vec<4, glm_Float>(q.x, q.y, q.z, q.w);
        }
        else
#endif
        {
          m.m44[dim - 1] = glm_vecvalue(val).v4;
        }
        break;
      }
      default: {
        return raw ? glm_runerror(L, INVALID_MATRIX_DIMENSIONS)
                   : glm_finishset(L, obj, key, val);
      }
    }

    m.size += (expanding ? 1 : 0);
    return 1;
  }
  else if (ttisnil(val)) {  // Attempt to shrink the dimension of the matrix
    if (dim == m.size && dim > 2) {  // Matrices must have at least two columns; >= 2x2
      m.size--;
      return 1;
    }
    return raw ? glm_runerror(L, LABEL_MATRIX " must have at least two columns")
               : glm_finishset(L, obj, key, val);
  }
  return raw ? glm_runerror(L, "attempt to set a " LABEL_MATRIX " value with an incorrect index")
             : glm_finishset(L, obj, key, val);
}

GCMatrix *glmMat_new(lua_State *L) {
  GCObject *o = luaC_newobj(L, LUA_VMATRIX, sizeof(GCMatrix));
  GCMatrix *mat = gco2mat(o);
  glm_mat_boundary(&mat->mat4) = glm::identity<glm::mat<4, 4, glm_Float>>();
  return mat;
}

int glmMat_rawgeti(const TValue *obj, lua_Integer n, StkId res) {
  if (matgeti(obj, n, res) == LUA_TNONE)
    setnilvalue(s2v(res));
  return ttypetag(s2v(res));
}

int glmMat_rawget(const TValue *obj, TValue *key, StkId res) {
  if (ttisnumber(key))
    return glmMat_rawgeti(obj, glm_tointeger(key), res);
  else {
    setnilvalue(s2v(res));
    return LUA_TNIL;
  }
}

void glmMat_rawset(lua_State *L, const TValue *obj, TValue *key, TValue *val) {
  glmMat_auxset(L, obj, key, val, true);
}

void glmMat_get(lua_State *L, const TValue *obj, TValue *key, StkId res) {
  if (!ttisnumber(key) || matgeti(obj, glm_tointeger(key), res) == LUA_TNONE)
    luaV_finishget(L, obj, key, res, NULL);
}

void glmMat_geti(lua_State *L, const TValue *obj, lua_Integer c, StkId res) {
  if (matgeti(obj, c, res) == LUA_TNONE) {
    TValue key;
    setivalue(&key, c);
    luaV_finishget(L, obj, &key, res, NULL);
  }
}

void glmMat_set(lua_State *L, const TValue *obj, TValue *key, TValue *val) {
  glmMat_auxset(L, obj, key, val, false);
}

void glmMat_seti(lua_State *L, const TValue *obj, lua_Integer c, TValue *val) {
  TValue key;
  setivalue(&key, c);
  glmMat_auxset(L, obj, &key, val, false);
}

void glmMat_objlen(const TValue *obj, StkId res) {
  setivalue(s2v(res), static_cast<lua_Integer>(mvalue(obj).size));
}

int glmMat_tostr(const TValue *obj, char *buff, size_t len) {
  int copy = 0;
  const glmMatrix &m = glm_mvalue(obj);
  switch (m.size) {
    case 2: {
      switch (m.secondary) {
        case 2: copy = glm::detail::format_type(buff, len, m.m22); break;
        case 3: copy = glm::detail::format_type(buff, len, m.m23); break;
        case 4: copy = glm::detail::format_type(buff, len, m.m24); break;
        default:
          break;
      }
      break;
    }
    case 3: {
      switch (m.secondary) {
        case 2: copy = glm::detail::format_type(buff, len, m.m32); break;
        case 3: copy = glm::detail::format_type(buff, len, m.m33); break;
        case 4: copy = glm::detail::format_type(buff, len, m.m34); break;
        default:
          break;
      }
      break;
    }
    case 4: {
      switch (m.secondary) {
        case 2: copy = glm::detail::format_type(buff, len, m.m42); break;
        case 3: copy = glm::detail::format_type(buff, len, m.m43); break;
        case 4: copy = glm::detail::format_type(buff, len, m.m44); break;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }

  lua_assert(copy >= 0);
  return copy;
}

int glmMat_next(const TValue *obj, StkId key) {
  TValue *key_value = s2v(key);
  if (ttisnil(key_value)) {
    setivalue(key_value, 1);
    glmMat_rawgeti(obj, 1, key + 1);
    return 1;
  }
  else if (ttisnumber(key_value)) {
    const lua_Integer l_nextIdx = glm_tointeger(key_value) + 1;
    const glm::length_t nextIdx = i_glmlen(l_nextIdx);
    if (nextIdx >= 1 && nextIdx <= mvalue(obj).size) {
      setivalue(key_value, l_nextIdx);  // Iterator values are 1-based
      glmMat_rawgeti(obj, l_nextIdx, key + 1);
      return 1;
    }
  }
  return 0;
}

int glmMat_equalObj(lua_State *L, const TValue *o1, const TValue *o2) {
  bool result = false;
  const glmMatrix &m = glm_mvalue(o1);
  const glmMatrix &other_m = glm_mvalue(o2);
  if (m.size == other_m.size && m.secondary == other_m.secondary) {
    switch (m.size) {
      case 2: {
        switch (m.secondary) {
          case 2: result = _glmeq(m.m22, other_m.m22); break;
          case 3: result = _glmeq(m.m23, other_m.m23); break;
          case 4: result = _glmeq(m.m24, other_m.m24); break;
          default:
            break;
        }
        break;
      }
      case 3: {
        switch (m.secondary) {
          case 2: result = _glmeq(m.m32, other_m.m32); break;
          case 3: result = _glmeq(m.m33, other_m.m33); break;
          case 4: result = _glmeq(m.m34, other_m.m34); break;
          default:
            break;
        }
        break;
      }
      case 4: {
        switch (m.secondary) {
          case 2: result = _glmeq(m.m42, other_m.m42); break;
          case 3: result = _glmeq(m.m43, other_m.m43); break;
          case 4: result = _glmeq(m.m44, other_m.m44); break;
          default:
            break;
        }
        break;
      }
      default:
        break;
    }
  }

  if (result == false && L != GLM_NULLPTR) {
    const TValue *tm = luaT_gettmbyobj(L, o1, TM_EQ);
    if (!notm(tm)) {
      luaT_callTMres(L, tm, o1, o2, L->top);  /* call TM */
      result = !l_isfalse(s2v(L->top));
    }
  }
  return result;
}

LUA_API int glm_unpack_matrix(lua_State *L, int idx) {
  luaL_checkstack(L, 4, "matrix unpack");

  const TValue *obj = glm_index2value(L, idx);
  if (ttismatrix(obj)) {
    const glmMatrix &m = glm_mvalue(obj);
    for (glm::length_t i = 0; i < m.size; ++i) {
      switch (m.secondary) {
        case 2: glm_pushvec2(L, m.m42[i]); break;
        case 3: glm_pushvec3(L, m.m43[i]); break;
        case 4: glm_pushvec4(L, m.m44[i]); break;
        default: {
          lua_pushnil(L);
          break;
        }
      }
    }
    return cast_int(m.size);
  }
  return 0;
}

/* }================================================================== */

/*
** {==================================================================
** GLM Interface
** ===================================================================
*/

/// <summary>
/// Generalized TValue to glm::vec conversion; using glmVector.Get to implicitly
/// handle type conversions.
/// </summary>
template <glm::length_t D, typename T>
static glm::vec<D, T> glm_tovec(lua_State *L, int idx) {
  glm::vec<D, glm_Float> result(0);

  const TValue *o = glm_index2value(L, idx);
  if (ttisvector(o) && glm_dimensions(ttypetag(o)) >= D)
    glm_vvalue(o).Get(result);
  return result;
}

/// <summary>
/// Generalized TValue to glm::mat conversion; using glmMatrix.Get to implicitly
/// handle type conversions.
/// </summary>
template <glm::length_t C, glm::length_t R, typename T>
static glm::mat<C, R, T> glm_tomat(lua_State *L, int idx) {
  glm::mat<C, R, T> result = glm::identity<glm::mat<C, R, glm_Float>>();

  const TValue *o = glm_index2value(L, idx);
  if (ttismatrix(o)) {
    const glmMatrix &m = glm_mvalue(o);
    if (m.size >= C && m.secondary == R)
      m.Get(result);
  }
  return result;
}

LUA_API int glm_pushvec(lua_State *L, const glmVector &v, glm::length_t dimensions) {
  const lu_byte variant = glm_variant(dimensions);
  if (variant == LUA_VVECTOR1)
    lua_pushnumber(L, cast_num(v.v1.x));
  else if (novariant(variant) == LUA_TVECTOR) {
    lua_lock(L);
    glm_setvvalue2s(L->top, v, variant);
    api_incr_top(L);
    lua_unlock(L);
  }
  else {
#if defined(LUA_USE_APICHECK)
    luaG_runerror(L, INVALID_VECTOR_TYPE);
#endif
    return 0;
  }
  return 1;
}

LUA_API int glm_pushquat_(lua_State *L, const glmVector &q) {
  lua_lock(L);
  glm_setvvalue2s(L->top, q, LUA_VQUAT);
  api_incr_top(L);
  lua_unlock(L);
  return 1;
}

LUA_API int glm_pushmat(lua_State *L, const glmMatrix &m) {
  if (m.size < 2 || m.size > 4 || m.secondary < 2 || m.secondary > 4) {
#if defined(LUA_USE_APICHECK)
    luaG_runerror(L, INVALID_MATRIX_DIMENSIONS);
#endif
    return 0;
  }

  lua_lock(L);
  GCMatrix *mat = glmMat_new(L);
  glm_mat_boundary(&mat->mat4) = m;
  glm_setmvalue2s(L, L->top, mat);
  api_incr_top(L);
  luaC_checkGC(L);
  lua_unlock(L);
  return 1;
};

LUA_API bool glm_isvector(lua_State *L, int idx, glm::length_t &size) {
  const TValue *o = glm_index2value(L, idx);
  if (ttisvector(o) && !ttisquat(o)) {
    size = glm_dimensions(ttypetag(o));
    return true;
  }
  else if (ttisnumber(o)) {
    size = 1;
    return true;
  }
  return false;
}

LUA_API bool glm_isquat(lua_State *L, int idx) {
  const TValue *o = glm_index2value(L, idx);
  return ttisquat(o);
}

LUA_API bool glm_ismatrix(lua_State *L, int idx, glm::length_t &size, glm::length_t &secondary) {
  const TValue *o = glm_index2value(L, idx);
  if (ttismatrix(o)) {
    size = mvalue(o).size;
    secondary = mvalue(o).secondary;
    return true;
  }
  return false;
}

LUA_API int glm_pushvec1(lua_State *L, const glm::vec<1, glm_Float> &v) { lua_pushnumber(L, cast_num(v.x)); return 1; }
LUA_API int glm_pushvec2(lua_State *L, const glm::vec<2, glm_Float> &v) { return glm_pushvec(L, glmVector(v), 2); }
LUA_API int glm_pushvec3(lua_State *L, const glm::vec<3, glm_Float> &v) { return glm_pushvec(L, glmVector(v), 3); }
LUA_API int glm_pushvec4(lua_State *L, const glm::vec<4, glm_Float> &v) { return glm_pushvec(L, glmVector(v), 4); }
LUA_API int glm_pushquat(lua_State *L, const glm::qua<glm_Float> &q) { return glm_pushquat_(L, glmVector(q)); }

LUA_API glm::vec<1, glm_Float> glm_tovec1(lua_State *L, int idx) { return glm::vec<1, glm_Float>(cast_glmfloat(lua_tonumber(L, idx))); }
LUA_API glm::vec<2, glm_Float> glm_tovec2(lua_State *L, int idx) { return glm_tovec<2, glm_Float>(L, idx); }
LUA_API glm::vec<3, glm_Float> glm_tovec3(lua_State *L, int idx) { return glm_tovec<3, glm_Float>(L, idx); }
LUA_API glm::vec<4, glm_Float> glm_tovec4(lua_State *L, int idx) { return glm_tovec<4, glm_Float>(L, idx); }
LUA_API glm::qua<glm_Float> glm_toquat(lua_State *L, int idx) {
  const TValue *o = glm_index2value(L, idx);
  return ttisquat(o) ? glm_quatvalue(o).q : glm::quat_identity<glm_Float, glm::defaultp>();
}

LUA_API int glm_pushmat2x2(lua_State *L, const glm::mat<2, 2, glm_Float> &m) { return glm_pushmat(L, glmMatrix(m)); }
LUA_API int glm_pushmat2x3(lua_State *L, const glm::mat<2, 3, glm_Float> &m) { return glm_pushmat(L, glmMatrix(m)); }
LUA_API int glm_pushmat2x4(lua_State *L, const glm::mat<2, 4, glm_Float> &m) { return glm_pushmat(L, glmMatrix(m)); }
LUA_API int glm_pushmat3x2(lua_State *L, const glm::mat<3, 2, glm_Float> &m) { return glm_pushmat(L, glmMatrix(m)); }
LUA_API int glm_pushmat3x3(lua_State *L, const glm::mat<3, 3, glm_Float> &m) { return glm_pushmat(L, glmMatrix(m)); }
LUA_API int glm_pushmat3x4(lua_State *L, const glm::mat<3, 4, glm_Float> &m) { return glm_pushmat(L, glmMatrix(m)); }
LUA_API int glm_pushmat4x2(lua_State *L, const glm::mat<4, 2, glm_Float> &m) { return glm_pushmat(L, glmMatrix(m)); }
LUA_API int glm_pushmat4x3(lua_State *L, const glm::mat<4, 3, glm_Float> &m) { return glm_pushmat(L, glmMatrix(m)); }
LUA_API int glm_pushmat4x4(lua_State *L, const glm::mat<4, 4, glm_Float> &m) { return glm_pushmat(L, glmMatrix(m)); }

LUA_API glm::mat<2, 2, glm_Float> glm_tomat2x2(lua_State *L, int idx) { return glm_tomat<2, 2, glm_Float>(L, idx); }
LUA_API glm::mat<2, 3, glm_Float> glm_tomat2x3(lua_State *L, int idx) { return glm_tomat<2, 3, glm_Float>(L, idx); }
LUA_API glm::mat<2, 4, glm_Float> glm_tomat2x4(lua_State *L, int idx) { return glm_tomat<2, 4, glm_Float>(L, idx); }
LUA_API glm::mat<3, 2, glm_Float> glm_tomat3x2(lua_State *L, int idx) { return glm_tomat<3, 2, glm_Float>(L, idx); }
LUA_API glm::mat<3, 3, glm_Float> glm_tomat3x3(lua_State *L, int idx) { return glm_tomat<3, 3, glm_Float>(L, idx); }
LUA_API glm::mat<3, 4, glm_Float> glm_tomat3x4(lua_State *L, int idx) { return glm_tomat<3, 4, glm_Float>(L, idx); }
LUA_API glm::mat<4, 2, glm_Float> glm_tomat4x2(lua_State *L, int idx) { return glm_tomat<4, 2, glm_Float>(L, idx); }
LUA_API glm::mat<4, 3, glm_Float> glm_tomat4x3(lua_State *L, int idx) { return glm_tomat<4, 3, glm_Float>(L, idx); }
LUA_API glm::mat<4, 4, glm_Float> glm_tomat4x4(lua_State *L, int idx) { return glm_tomat<4, 4, glm_Float>(L, idx); }

LUA_API const char *glm_typename(lua_State *L, int idx) {
  const TValue *o = glm_index2value(L, idx);
  switch (ttypetag(o)) {
    case LUA_VNUMFLT: return LABEL_NUMBER;
    case LUA_VNUMINT: return LABEL_INTEGER;
    case LUA_VVECTOR2: return LABEL_VECTOR2;
    case LUA_VVECTOR3: return LABEL_VECTOR3;
    case LUA_VVECTOR4: return LABEL_VECTOR4;
    case LUA_VQUAT: return LABEL_QUATERN;
    case LUA_VMATRIX: return LABEL_MATRIX;
    default:
      return "Unknown GLM Type";
  }
}

LUA_API const char *glm_pushstring(lua_State *L, int idx) {
  const TValue *o = glm_index2value(L, idx);
  if (ttisinteger(o))
    return lua_pushfstring(L, LUA_INTEGER_FMT, ivalue(o));
  else if (ttisfloat(o))
    return lua_pushfstring(L, LUA_NUMBER_FMT, static_cast<LUAI_UACNUMBER>(lua_tonumber(L, idx)));
  else if (ttisvector(o)) {
    char buff[GLM_STRING_BUFFER];
    const int len = glmVec_tostr(o, buff, GLM_STRING_BUFFER);
    return lua_pushlstring(L, buff, cast_sizet(len < 0 ? 0 : len));
  }
  else if (ttismatrix(o)) {
    char buff[GLM_STRING_BUFFER];
    const int len = glmMat_tostr(o, buff, GLM_STRING_BUFFER);
    return lua_pushlstring(L, buff, cast_sizet(len < 0 ? 0 : len));
  }
  return lua_pushliteral(L, "nil");
}

/* }================================================================== */

/*
** {==================================================================
** Deprecated gritLua API
** ===================================================================
*/

/// <summary>
/// Improved slerp implementation for generalized vectors.
/// </summary>
namespace glm {
  template<length_t L, typename T, qualifier Q>
  GLM_FUNC_QUALIFIER vec<L, T, Q> __objslerp(vec<L, T, Q> const &x, vec<L, T, Q> const &y, T const &a) {
    const T CosAlpha = dot(x, y);

    // Perform a linear interpolation when CosAlpha is close to 1 to avoid side
    // effect of sin(angle) becoming a zero denominator
    if (CosAlpha > static_cast<T>(1) - epsilon<T>())
      return mix(x, y, a);
    else {
      const T Alpha = acos(CosAlpha);  // get angle (0 -> pi)
      const T SinAlpha = sin(Alpha);  // get sine of angle between vectors (0 -> 1)
      const T t1 = sin((static_cast<T>(1) - a) * Alpha) / SinAlpha;
      const T t2 = sin(a * Alpha) / SinAlpha;
      return x * t1 + y * t2;
    }
  }

  /// Extensions to GLM to allow more complex matrix multiplication operations

  template<typename T, typename T2>
  GLM_FUNC_QUALIFIER T __objFloorDivide(const T &x, const T2 &y) {
    return floor(x / y);
  }

  template<length_t L, typename T, qualifier Q>
  GLM_FUNC_QUALIFIER vec<L, T, Q> __objPow(const vec<L, T, Q> &v, const T exp) {
    return pow(v, vec<L, T, Q>(exp));
  }
}

lua_Integer luaO_HashString(const char* string, size_t length, int ignore_case) {
  unsigned int hash = 0;
  for (size_t i = 0; i < length; ++i) {
    hash += (ignore_case ? string[i] : tolower(string[i]));
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }

  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  /* @TODO: Eventually avoid sign-extension issues. If ever... */
  return (lua_Integer)(int)hash;
}

/* gritLua functions stored in lbaselib; considered deprecated */

LUA_API lua_Integer glm_tohash(lua_State *L, int idx, int ignore_case) {
  const TValue *o = glm_index2value(L, idx);
  if (ttisstring(o))
    return luaO_HashString(svalue(o), vslen(o), ignore_case);
  else if (ttisboolean(o))
    return ttistrue(o) ? 1 : 0;
  else if (ttisnumber(o)) {
    lua_Integer res = 0;
    return tointeger(o, &res) ? res : 0;
  }
  return 0;
}

LUA_API int glmVec_dot(lua_State *L) {
  const TValue *x = glm_index2value(L, 1);
  const TValue *y = glm_index2value(L, 2);
  if (ttisinteger(x) && ttisinteger(y))
    lua_pushinteger(L, ivalue(x) * ivalue(y));
  else if (ttisnumber(x) && ttisnumber(y))
    lua_pushnumber(L, nvalue(x) * nvalue(y));
  else if (ttisquat(x) && ttisquat(y))
    lua_pushnumber(L, cast_num(glm::dot(glm_quatvalue(x).q, glm_quatvalue(y).q)));
  else if (ttisvector(x) && ttypetag(x) == ttypetag(y)) {
    switch (ttypetag(x)) {
      case LUA_VVECTOR2: lua_pushnumber(L, cast_num(glm::dot(glm_vecvalue(x).v2, glm_vecvalue(y).v2))); break;
      case LUA_VVECTOR3: lua_pushnumber(L, cast_num(glm::dot(glm_vecvalue(x).v3, glm_vecvalue(y).v3))); break;
      case LUA_VVECTOR4: lua_pushnumber(L, cast_num(glm::dot(glm_vecvalue(x).v4, glm_vecvalue(y).v4))); break;
      default:
        lua_pushnumber(L, 0);
        break;
    }
  }
  else {
    return luaL_typeerror(L, 1, LABEL_NUMBER " or " LABEL_VECTOR " type");
  }
  return 1;
}

LUA_API int glmVec_cross(lua_State *L) {
  const TValue *x = glm_index2value(L, 1);
  const TValue *y = glm_index2value(L, 2);
  switch (ttypetag(x)) {
    case LUA_VVECTOR2: {
      if (ttypetag(y) == LUA_VVECTOR2) {
        lua_pushnumber(L, cast_num(glm::cross(glm_vecvalue(x).v2, glm_vecvalue(y).v2)));
        return 1;
      }
      return luaL_typeerror(L, 2, LABEL_VECTOR2);
    }
    case LUA_VVECTOR3: {
      if (ttypetag(y) == LUA_VQUAT)
        return glm_pushvec3(L, glm::cross(glm_vecvalue(x).v3, glm_quatvalue(y).q));
      if (ttypetag(y) == LUA_VVECTOR3)
        return glm_pushvec3(L, glm::cross(glm_vecvalue(x).v3, glm_vecvalue(y).v3));
      return luaL_typeerror(L, 2, LABEL_VECTOR3 " or " LABEL_QUATERN);
    }
    case LUA_VQUAT: {
      if (ttypetag(y) == LUA_VQUAT)
        return glm_pushquat(L, glm::cross(glm_quatvalue(x).q, glm_quatvalue(y).q));
      if (ttypetag(y) == LUA_VVECTOR3)
        return glm_pushvec3(L, glm::cross(glm_quatvalue(x).q, glm_vecvalue(y).v3));
      return luaL_typeerror(L, 2, LABEL_VECTOR3 " or " LABEL_QUATERN);
    }
    default:
      break;
  }
  return luaL_typeerror(L, 1, LABEL_VECTOR2 ", " LABEL_VECTOR3 ", or " LABEL_QUATERN);
}

LUA_API int glmVec_inverse(lua_State *L) {
  const TValue *x = glm_index2value(L, 1);
  if (ttisquat(x))
    return glm_pushquat(L, glm::inverse(glm_quatvalue(x).q));
  else if (ttismatrix(x)) {
    const glmMatrix &m = glm_mvalue(x);
    if (m.size == m.secondary) {
      switch (m.size) {
        case 2: return glm_pushmat2x2(L, glm::inverse(m.m22));
        case 3: return glm_pushmat3x3(L, glm::inverse(m.m33));
        case 4: return glm_pushmat4x4(L, glm::inverse(m.m44));
        default:
          break;
      }
    }
  }
  return luaL_typeerror(L, 1, LABEL_QUATERN ", or " LABEL_SYMMETRIC_MATRIX);
}

LUA_API int glmVec_normalize(lua_State *L) {
  const TValue *x = glm_index2value(L, 1);
  switch (ttypetag(x)) {
    case LUA_VVECTOR2: return glm_pushvec2(L, glm::normalize(glm_vecvalue(x).v2));
    case LUA_VVECTOR3: return glm_pushvec3(L, glm::normalize(glm_vecvalue(x).v3));
    case LUA_VVECTOR4: return glm_pushvec4(L, glm::normalize(glm_vecvalue(x).v4));
    case LUA_VQUAT: return glm_pushquat(L, glm::normalize(glm_quatvalue(x).q));
    default:
      break;
  }
  return luaL_typeerror(L, 1, LABEL_VECTOR " or " LABEL_QUATERN);
}

LUA_API int glmVec_slerp(lua_State *L) {
  const TValue *x = glm_index2value(L, 1);
  const TValue *y = glm_index2value(L, 2);
  const TValue *a = glm_index2value(L, 3);
  if (ttypetag(x) == ttypetag(y) && ttype(a) == LUA_TNUMBER) {
    const glm_Float t = cast_glmfloat(nvalue(a));
    switch (ttypetag(x)) {
      case LUA_VVECTOR2: return glm_pushvec2(L, glm::__objslerp(glm_vecvalue(x).v2, glm_vecvalue(y).v2, t));
      case LUA_VVECTOR3: return glm_pushvec3(L, glm::__objslerp(glm_vecvalue(x).v3, glm_vecvalue(y).v3, t));
      case LUA_VVECTOR4: return glm_pushvec4(L, glm::__objslerp(glm_vecvalue(x).v4, glm_vecvalue(y).v4, t));
      case LUA_VQUAT: return glm_pushquat(L, glm::slerp(glm_quatvalue(x).q, glm_quatvalue(y).q, t));
      default:
        break;
    }
  }
  return luaL_error(L, "slerp(v, v, a) or slerp(q, q, a) expected");
}

/*
** Parse the provided table (as a TValue) as a vector, i.e., check if the table
** has numeric 'x', 'y', 'z', and 'w' fields and count how many... Returning
** a valid vector variant (bounded by the first dimension that is nil or
** not-numeric) the table has.
*/
static glm::length_t glmH_tovector(lua_State *L, const TValue *o, glmVector *v) {
  static const char *const dims[] = { "x", "y", "z", "w" };

  glm::length_t count = 0;
  for (int i = 0; i < 4; ++i) {
    TString *key = luaS_newlstr(L, dims[i], 1);  /* luaS_newliteral */
    const TValue *slot = luaH_getstr(hvalue(o), key);
    if (ttisnumber(slot)) {
      count++;
      if (v != GLM_NULLPTR)
        v->v4[i] = cast_glmfloat(nvalue(slot));
    }
  }
  return (count > 0) ? glm_variant(count) : LUA_TNIL;
}

LUA_API lua_Integer lua_ToHash (lua_State *L, int idx, int ignore_case) {
  return glm_tohash(L, idx, ignore_case);
}

LUA_API int luaVec_dimensions (int rtt) {
  return (rtt == LUA_VVECTOR1) ? 1 : cast_int(glm_dimensions(rtt));
}

LUA_API int lua_isvector (lua_State *L, int idx, int flags) {
  const TValue *o = glm_index2value(L, idx);
  int variant = LUA_TNIL;
  if (ttisvector(o))
    variant = ttypetag(o);
  else if (ttisnumber(o) && (flags & V_NONUMBER) == 0)
    variant = LUA_VVECTOR1;
  else if (ttistable(o) && (flags & V_PARSETABLE) != 0) {
    lua_lock(L);
    variant = glmH_tovector(L, o, GLM_NULLPTR);
    lua_unlock(L);
  }
  return variant;
}

LUA_API int lua_tovector (lua_State *L, int idx, int flags, lua_Float4 *f4) {
  const TValue *o = glm_index2value(L, idx);
  glmVector v;
  int variant = LUA_TNIL;
  if (ttisvector(o)) {
    v = glm_vvalue(o);
    variant = ttypetag(o);
  }
  else if ((flags & V_PARSETABLE) != 0 && ttistable(o)) {
    lua_lock(L);
    variant = glmH_tovector(L, o, &v);
    lua_unlock(L);
  }
  else if ((flags & V_NONUMBER) == 0 && ttisnumber(o)) {
    lua_Number n = 0;  // @TODO Warn of potential down-casting.
    if (tonumberns(o, n)) {
      v.v4.x = cast_glmfloat(n);
      variant = LUA_VVECTOR1;
    }
  }

  if (f4 != GLM_NULLPTR && variant != LUA_TNIL) {
    if (variant == LUA_VVECTOR1)
      f4->x = v.v4.x;
    else if (novariant(variant) == LUA_TVECTOR) {  // Support GLM_FORCE_QUAT_DATA_WXYZ
      f4->x = ((variant == LUA_VQUAT) ? v.q.x : v.v4.x);
      f4->y = ((variant == LUA_VQUAT) ? v.q.y : v.v4.y);
      f4->z = ((variant == LUA_VQUAT) ? v.q.z : v.v4.z);
      f4->w = ((variant == LUA_VQUAT) ? v.q.w : v.v4.w);
    }
  }
  return variant;
}

LUA_API void lua_pushvector (lua_State *L, lua_Float4 f4, int variant) {
  variant = withvariant(variant);
  if (novariant(variant) != LUA_TVECTOR) {
#if defined(LUA_USE_APICHECK)
    luaG_runerror(L, INVALID_VECTOR_TYPE);
#else
    lua_pushnil(L);
#endif
  }
  else if (variant == LUA_VVECTOR1)  // Implicit vector1
    lua_pushnumber(L, cast_num(f4.x));
  else if (variant == LUA_VQUAT)  // Support GLM_FORCE_QUAT_DATA_WXYZ
    glm_pushquat_(L, glmVector(glm::qua<glm_Float>(f4.w, f4.x, f4.y, f4.z)));
  else
    glm_pushvec(L, glmVector(glm::vec<4, glm_Float>(f4.x, f4.y, f4.z, f4.w)), glm_dimensions(variant));
}

LUA_API int lua_ismatrix (lua_State *L, int idx, int *size, int *secondary) {
  const TValue *o = glm_index2value(L, idx);
  if (ttismatrix(o)) {
    if (size != GLM_NULLPTR) *size = cast_int(mvalue(o).size);
    if (secondary != GLM_NULLPTR) *secondary = cast_int(mvalue(o).secondary);
    return 1;
  }
  return 0;
}

LUA_API int lua_tomatrix (lua_State *L, int idx, lua_Mat4 *matrix) {
  const TValue *o = glm_index2value(L, idx);
  if (ttismatrix(o) && matrix != GLM_NULLPTR) {
    *matrix = mvalue(o);
    return 1;
  }
  return 0;
}

LUA_API int lua_pushmatrix (lua_State *L, lua_Mat4 *matrix) {
  if (matrix == GLM_NULLPTR
      || matrix->size < 2 || matrix->size > 4
      || matrix->secondary < 2 || matrix->secondary > 4) {
#if defined(LUA_USE_APICHECK)
    luaG_runerror(L, INVALID_MATRIX_DIMENSIONS);
#endif
    return 0;
  }
  return glm_pushmat(L, glmMatrixBoundary(*matrix).glm);
}

/* }================================================================== */

/*
** {==================================================================
** Constructors
** ===================================================================
*/

/// <summary>
/// Statically cast a tagged value to the specified type parameter
/// </summary>
/// <param name="valid">A referent that indicates whether the operation succeeded.</param>
template<typename T>
static bool glm_castvalue(const TValue *value, T &out) {
  switch (ttypetag(value)) {
    case LUA_VTRUE: out = static_cast<T>(1); break;
    case LUA_VFALSE: out = static_cast<T>(0); break;
    case LUA_VNUMINT: out = static_cast<T>(ivalue(value)); break;
    case LUA_VNUMFLT: out = static_cast<T>(fltvalue(value)); break;
    default: {
      out = static_cast<T>(0);
      return false;
    }
  }
  return true;
}

/// <summary>
/// Unpack a tagged value into a vector "v" starting at offset "v_idx"
/// </summary>
template<typename T>
static glm::length_t PopulateVectorObject(lua_State *L, int idx, glm::vec<4, T> &vec, glm::length_t v_idx, glm::length_t v_desired, const TValue *value) {
  if (glm_castvalue(value, vec[v_idx]))  // Primitive type: cast & store it.
    return 1;
  else if (ttisvector(value)) {  // Vector: concatenate components values.
    const glmVector &v = glm_vvalue(value);
    const glm::length_t length = glm_dimensions(ttypetag(value));
    if ((v_idx + length) > v_desired)
      return luaL_argerror(L, idx, INVALID_VECTOR_DIMENSIONS);

#if defined(GLM_FORCE_QUAT_DATA_WXYZ)  // Support GLM_FORCE_QUAT_DATA_WXYZ (access by array)
    if (ttisquat(value)) {
      vec[v_idx++] = static_cast<T>(v.q[0]);
      vec[v_idx++] = static_cast<T>(v.q[1]);
      vec[v_idx++] = static_cast<T>(v.q[2]);
      vec[v_idx++] = static_cast<T>(v.q[3]);
    }
    else
#endif
    {
      for (glm::length_t j = 0; j < length; ++j)
        vec[v_idx++] = static_cast<T>(v.v4[j]);
    }
    return length;
  }
  else if (ttistable(value)) {  // Array: concatenate values.
    const glm::length_t t_len = i_glmlen(luaH_getn(hvalue(value)));
    if ((v_idx + t_len) > v_desired)
      return luaL_argerror(L, idx, INVALID_VECTOR_DIMENSIONS);

    for (glm::length_t j = 1; j <= t_len; ++j) {
      const TValue *t_val = luaH_getint(hvalue(value), i_luaint(j));
      if (!glm_castvalue(t_val, vec[v_idx++]))  // Primitive type: cast & store it.
        return luaL_argerror(L, idx, INVALID_VECTOR_TYPE);
    }
    return t_len;
  }

  return luaL_argerror(L, idx, INVALID_VECTOR_TYPE);
}

/// <summary>
/// Generic matrix population/construction function: this function will iterate
/// over the current Lua stack, expecting a column vector for each dimension of
/// the matrix.
///
/// A desired/expected dimensionality of the matrix can also be passed to this
/// function, or, have those parameters be equal to -1 and have its dimensions
/// inferred from (1) the number of supplied column vectors; and (2) the
/// dimensionality of each column vector.
/// </summary>
static bool PopulateMatrix(lua_State *L, int idx, bool fixed_size, glmMatrix &m, glm::length_t &outSize, glm::length_t &outSecondary) {
  glm::length_t size = 0, secondary = 0;

  // Maximum number of stack values to parse from the starting "idx" idx = lua_absindex(L, idx);
  const int stack_count = (_gettop(L) - idx + 1);
  const TValue *o = glm_index2value(L, idx);

  // If the first object is a quaternion, cast it to the arbitrarily sized
  // matrix. This logic follows glm::toMat3 and glm::toMat4 and uses the
  // constructors to down/up-cast the matrix
  if (stack_count == 1 && ttisquat(o)) {
    size = m.size;
    secondary = m.secondary;
    m.m44 = glm::mat4_cast<glm_Float, glm::defaultp>(glm_quatvalue(o).q);
  }

  // If the first object is a matrix, down/up-cast it.
  if (stack_count == 1 && ttismatrix(o)) {
    const glmMatrix &_m = glm_mvalue(o);

    size = fixed_size ? m.size : _m.size;
    secondary = fixed_size ? m.secondary : _m.secondary;
    switch (_m.size) {
      case 2: {
        switch (_m.secondary) {
          case 2: m.m44 = glm::mat<4, 4, glm_Float>(_m.m22); break;
          case 3: m.m44 = glm::mat<4, 4, glm_Float>(_m.m23); break;
          case 4: m.m44 = glm::mat<4, 4, glm_Float>(_m.m24); break;
          default:
            return false;
        }
        break;
      }
      case 3: {
        switch (_m.secondary) {
          case 2: m.m44 = glm::mat<4, 4, glm_Float>(_m.m32); break;
          case 3: m.m44 = glm::mat<4, 4, glm_Float>(_m.m33); break;
          case 4: m.m44 = glm::mat<4, 4, glm_Float>(_m.m34); break;
          default:
            return false;
        }
        break;
      }
      case 4: {
        switch (_m.secondary) {
          case 2: m.m44 = glm::mat<4, 4, glm_Float>(_m.m42); break;
          case 3: m.m44 = glm::mat<4, 4, glm_Float>(_m.m43); break;
          case 4: m.m44 = _m.m44; break;
          default:
            return false;
        }
        break;
      }
      default:
        return false;
    }
  }
  // Otherwise, parse column vectors
  else {
    // If there is only one element to be parsed and it is a table, assume the
    // matrix is packed within an array; otherwise, use the elements on the stack.
    const bool as_table = stack_count == 1 && ttistable(o);

    const glm::length_t column_limit = as_table ? m.size : std::min(m.size, i_glmlen(stack_count));
    for (; size < column_limit; ++size) {
      glm::length_t v_size = 0;
      if (as_table) {  // An array contains all of the elements of a matrix.
        const TValue *value = luaH_getint(hvalue(o), i_luaint(size) + 1);
        v_size = ttisnil(value) ? 0 : PopulateVectorObject(L, idx, m.m44[size], 0, m.secondary, value);
      }
      else {
        const TValue *value = glm_index2value(L, idx);
        v_size = PopulateVectorObject(L, idx++, m.m44[size], 0, m.secondary, value);
      }

      if (v_size > 1) {  // Parse the column/row vector.
        if (secondary > 0 && secondary != v_size)  // Inconsistent dimensions
          return false;

        secondary = v_size;
      }
      else if (secondary == 0)
        return false;  // No/not-enough columns have been parsed
      else
        break;  // At least one column has been parsed.
    }
  }

  outSize = size;
  outSecondary = secondary;
  return (size >= 2 && size <= 4 && secondary >= 2 && secondary <= 4);
}

/// <summary>
/// Generic vector population/construction function.
///
/// This function will iterate over the current Lua stack and unpack its values;
/// returning the number of components of the vector populated and zero on
/// failure (e.g., exceeding row size or attempting to populate the vector with
/// an invalid type).
///
/// Unpacking Rules:
///   (1) A primitive type (float, int, bool) will be stored at v[X];
///   (2) A vector (of N dimensions) will have its contents stored at v[X],
///       v[X + 1], ..., v[X + N]; following x, y, z, w ordering (same applies
///       to quaternions);
///   (3) An array (of length N) will have contents started at v[X], v[X + 1], etc.
///   (4) Otherwise, a lua_error is thrown.
/// </summary>
/// <param name="L">Current Lua State</param>
/// <param name="desiredSize">Desired number of elements: [0, 4]</param>
/// <returns></returns>
template<typename T>
static int glm_createVector(lua_State *L, glm::length_t desiredSize = 0) {
  glm::vec<4, T> v(T(0));
  glm::length_t v_len = 0;

  // If the vector is of a fixed/desired size and only one non-table argument has been supplied
  const int top = _gettop(L);
  const TValue *o_f = glm_index2value(L, 1);
  if (desiredSize > 0 && top == 1 && glm_castvalue(o_f, v.x))
    return glm_pushvec(L, glmVector(glm::vec<4, T>(v.x)), desiredSize);

  // Maximum number of stack values to parse, starting from "idx"
  const glm::length_t v_max = desiredSize == 0 ? 4 : desiredSize;
  for (int i = 1; i <= top; ++i) {
    if (v_len >= v_max)  // Ensure at least one element can be packed into the vector;
      return luaL_argerror(L, i, INVALID_VECTOR_DIMENSIONS);

    v_len += PopulateVectorObject(L, i, v, v_len, v_max, glm_index2value(L, i));
  }

  if (desiredSize == 0 && v_len == 0)
    return luaL_error(L, LABEL_VECTOR " requires 1 to 4 numbers");
  else if (desiredSize != 0 && v_len != desiredSize)
    return luaL_error(L, LABEL_VECTOR "%d requires exactly %d number(s)", cast_int(desiredSize), cast_int(desiredSize));
  else if (v_len == 1) {
    GLM_IF_CONSTEXPR(std::is_same<T, bool>::value)
      lua_pushboolean(L, cast_int(v.x));
    else GLM_IF_CONSTEXPR(std::is_integral<T>::value)
      lua_pushinteger(L, i_luaint(v.x));
    else
      lua_pushnumber(L, cast_num(v.x));
    return 1;
  }

  return glm_pushvec(L, glmVector(v), v_len);
}

/// <summary>
/// Generalized create matrix.
///
/// TODO: Only operate on glm::mat<4, 4> to reduce template function/logic
/// duplication (i.e., one for each glm::mat pairing)
/// </summary>
template<typename T>
static int glm_createMatrix(lua_State *L, glm::length_t C, glm::length_t R) {
  const int top = _gettop(L);
  int start_idx = 0;
  bool success = false;

  glmMatrix result(glm::mat<4, 4, T>(T(0)));
  result.size = (C == 0) ? 4 : C;
  result.secondary = (R == 0) ? 4 : R;

  if (top == 0) {  // If there are no elements, return the identity matri
    success = true;
    result.m44 = glm::identity<glm::mat<4, 4, T>>();
  }
  else {  // If the first element is a matrix, recycle the collectible
    const TValue *o = glm_index2value(L, 1);
    start_idx = (top > 1 && ttismatrix(o)) ? 2 : 1;
    success = PopulateMatrix(L, start_idx, (C != 0 || R != 0), result, result.size, result.secondary);
  }

  if (success) {
    // Realign column-vectors, ensuring the matrix can be faithfully represented
    // by its m.mCR union value.
    switch (result.secondary) {
      case 2: result.m42 = glm::mat<4, 2, glm_Float>(result.m44); break;
      case 3: result.m43 = glm::mat<4, 3, glm_Float>(result.m44); break;
      case 4: result.m44 = glm::mat<4, 4, glm_Float>(result.m44); break;
      default:
        break;
    }

    if (start_idx > 1) { // Recycle the matrix
      const TValue *o = GLM_NULLPTR;

      lua_pushvalue(L, 1);
      lua_lock(L);
      o = glm_index2value(L, -1);
      glm_mat_boundary(mvalue_ref(o)) = result;
      lua_unlock(L);
      return 1;
    }
    return glm_pushmat(L, result);
  }
  return luaL_error(L, INVALID_MATRIX_STRUCTURE);
}

LUA_API int glmVec_vec(lua_State *L) { return glm_createVector<glm_Float>(L); }
LUA_API int glmVec_vec1(lua_State *L) { return glm_createVector<glm_Float>(L, 1); }
LUA_API int glmVec_vec2(lua_State *L) { return glm_createVector<glm_Float>(L, 2); }
LUA_API int glmVec_vec3(lua_State *L) { return glm_createVector<glm_Float>(L, 3); }
LUA_API int glmVec_vec4(lua_State *L) { return glm_createVector<glm_Float>(L, 4); }

LUA_API int glmVec_ivec(lua_State *L) { return glm_createVector<glm_Integer>(L); }
LUA_API int glmVec_ivec1(lua_State *L) { return glm_createVector<glm_Integer>(L, 1); }
LUA_API int glmVec_ivec2(lua_State *L) { return glm_createVector<glm_Integer>(L, 2); }
LUA_API int glmVec_ivec3(lua_State *L) { return glm_createVector<glm_Integer>(L, 3); }
LUA_API int glmVec_ivec4(lua_State *L) { return glm_createVector<glm_Integer>(L, 4); }

LUA_API int glmVec_bvec(lua_State *L) { return glm_createVector<bool>(L); }
LUA_API int glmVec_bvec1(lua_State *L) { return glm_createVector<bool>(L, 1); }
LUA_API int glmVec_bvec2(lua_State *L) { return glm_createVector<bool>(L, 2); }
LUA_API int glmVec_bvec3(lua_State *L) { return glm_createVector<bool>(L, 3); }
LUA_API int glmVec_bvec4(lua_State *L) { return glm_createVector<bool>(L, 4); }

LUA_API int glmMat_mat2x2(lua_State *L) { return glm_createMatrix<glm_Float>(L, 2, 2); }
LUA_API int glmMat_mat2x3(lua_State *L) { return glm_createMatrix<glm_Float>(L, 2, 3); }
LUA_API int glmMat_mat2x4(lua_State *L) { return glm_createMatrix<glm_Float>(L, 2, 4); }
LUA_API int glmMat_mat3x2(lua_State *L) { return glm_createMatrix<glm_Float>(L, 3, 2); }
LUA_API int glmMat_mat3x3(lua_State *L) { return glm_createMatrix<glm_Float>(L, 3, 3); }
LUA_API int glmMat_mat3x4(lua_State *L) { return glm_createMatrix<glm_Float>(L, 3, 4); }
LUA_API int glmMat_mat4x2(lua_State *L) { return glm_createMatrix<glm_Float>(L, 4, 2); }
LUA_API int glmMat_mat4x3(lua_State *L) { return glm_createMatrix<glm_Float>(L, 4, 3); }
LUA_API int glmMat_mat4x4(lua_State *L) { return glm_createMatrix<glm_Float>(L, 4, 4); }
LUA_API int glmMat_mat(lua_State *L) { return glm_createMatrix<glm_Float>(L, 0, 0); }

/// <summary>
/// Function written to bypass API overheads;
/// </summary>
LUA_API int glmVec_qua(lua_State *L) {
  const TValue *o1 = glm_index2value(L, 1);
  if (ttisnumber(o1)) {
    const TValue *o2 = glm_index2value(L, 2);
    if (ttisvector3(o2))  // <angle, axis>, degrees for gritLua compatibility
      return glm_pushquat(L, glm::angleAxis(cast_glmfloat(glm::radians(nvalue(o1))), glm_vecvalue(o2).v3));
    else if (ttisnumber(o2)) {  // <w, x, y, z>
      return glm_pushquat(L, glm::qua<glm_Float>(
        cast_glmfloat(nvalue(o1)),
        cast_glmfloat(nvalue(o2)),
        cast_glmfloat(luaL_checknumber(L, 3)),
        cast_glmfloat(luaL_checknumber(L, 4)))
      );
    }
    return luaL_error(L, "{w, x, y, z} or {angle, axis} expected");
  }
  else if (ttisvector3(o1)) {
    const TValue *o2 = glm_index2value(L, 2);
    if (!_isvalid(L, o2))  // <euler>
      return glm_pushquat(L, glm::qua<glm_Float>(glm_vecvalue(o1).v3));
    else if (ttisnumber(o2))  // <xyz, w>
      return glm_pushquat(L, glm::qua<glm_Float>(cast_glmfloat(nvalue(o2)), glm_vecvalue(o1).v3));
    else if (ttisvector3(o2))  // <from, to>
      return glm_pushquat(L, glm::qua<glm_Float>(glm_vecvalue(o1).v3, glm_vecvalue(o2).v3));
    return luaL_error(L, "{euler}, {from, to}, or {xyz, w} expected");
  }
  else if (ttisquat(o1)) {
    lua_pushvalue(L, 1);
    return 1;
  }
  else if (ttismatrix(o1)) {
    const glmMatrix &m = glm_mvalue(o1);
    if (m.size == m.secondary) {
      switch (m.size) {
        case 3: return glm_pushquat(L, glm::qua<glm_Float>(m.m33));
        case 4: return glm_pushquat(L, glm::qua<glm_Float>(m.m44));
        default:
          return luaL_typeerror(L, 1, LABEL_MATRIX "3x3 or " LABEL_MATRIX "4x4");
      }
    }
  }
  return luaL_typeerror(L, 1, LABEL_NUMBER ", " LABEL_VECTOR3 ", or " LABEL_MATRIX);
}

/* }================================================================== */

/*
** {==================================================================
** Intentionally ugly code
**
** @TODO: Profile/tune all statements below.
** ===================================================================
*/

#define glm_newmvalue(L, obj, x)      \
  LUA_MLM_BEGIN                       \
  GCMatrix *mat = glmMat_new(L);      \
  glm_mat_boundary(&mat->mat4) = (x); \
  glm_setmvalue2s(L, obj, mat);       \
  luaC_checkGC(L);                    \
  LUA_MLM_END

#define SCALAR(p1) (ttisinteger(p1) ? cast_glmfloat(ivalue(p1)) : cast_glmfloat(fltvalue(p1)))

/* F(vecN, vecN), e.g., vecN * vecN */
#define VECTOR_OPERATION(F, res, v, p2)                                              \
  LUA_MLM_BEGIN                                                                      \
  const glmVector &v2 = glm_vvalue((p2));                                            \
  switch (tt_p1) {                                                                   \
    case LUA_VVECTOR2: glm_setvvalue2s(res, F(v.v2, v2.v2), LUA_VVECTOR2); return 1; \
    case LUA_VVECTOR3: glm_setvvalue2s(res, F(v.v3, v2.v3), LUA_VVECTOR3); return 1; \
    case LUA_VVECTOR4: glm_setvvalue2s(res, F(v.v4, v2.v4), LUA_VVECTOR4); return 1; \
    default:                                                                         \
      break;                                                                         \
  }                                                                                  \
  LUA_MLM_END

/* F(vecN, value), e.g., vecN * 3 */
#define VECTOR_SCALAR_OPERATION(F, res, v, p2)                                   \
  LUA_MLM_BEGIN                                                                  \
  const glm_Float s = SCALAR((p2));                                              \
  switch (tt_p1) {                                                               \
    case LUA_VVECTOR2: glm_setvvalue2s(res, F(v.v2, s), LUA_VVECTOR2); return 1; \
    case LUA_VVECTOR3: glm_setvvalue2s(res, F(v.v3, s), LUA_VVECTOR3); return 1; \
    case LUA_VVECTOR4: glm_setvvalue2s(res, F(v.v4, s), LUA_VVECTOR4); return 1; \
    default:                                                                     \
      break;                                                                     \
  }                                                                              \
  LUA_MLM_END

/*
** Operations on integer vectors (or floating-point vectors that are int-casted).
**
** @TODO: Once int-vectors become natively supported, this will require a rewrite
*/
#define INT_VECTOR_OPERATION(F, res, v, p2)                                    \
  LUA_MLM_BEGIN                                                                \
  if (tt_p1 == tt_p2) {                                                        \
    const glmVector &v2 = glm_vecvalue(p2);                                    \
    switch (tt_p1) {                                                           \
      case LUA_VVECTOR2: glm_setvvalue2s(res, F(cast_vec2(v.v2, lua_Integer), cast_vec2(v2.v2, lua_Integer)), LUA_VVECTOR2); return 1; \
      case LUA_VVECTOR3: glm_setvvalue2s(res, F(cast_vec3(v.v3, lua_Integer), cast_vec3(v2.v3, lua_Integer)), LUA_VVECTOR3); return 1; \
      case LUA_VVECTOR4: glm_setvvalue2s(res, F(cast_vec4(v.v4, lua_Integer), cast_vec4(v2.v4, lua_Integer)), LUA_VVECTOR4); return 1; \
      default:                                                                 \
        break;                                                                 \
    }                                                                          \
  }                                                                            \
  else if (ttisinteger(p2)) {                                                  \
    const lua_Integer v2 = ivalue(p2);                                         \
    switch (tt_p1) {                                                           \
      case LUA_VVECTOR2: glm_setvvalue2s(res, F(cast_vec2(v.v2, lua_Integer), v2), LUA_VVECTOR2); return 1; \
      case LUA_VVECTOR3: glm_setvvalue2s(res, F(cast_vec3(v.v3, lua_Integer), v2), LUA_VVECTOR3); return 1; \
      case LUA_VVECTOR4: glm_setvvalue2s(res, F(cast_vec4(v.v4, lua_Integer), v2), LUA_VVECTOR4); return 1; \
      default:                                                                 \
        break;                                                                 \
    }                                                                          \
  }                                                                            \
  LUA_MLM_END

/*
** F(matNxM, matNxM): e.g., matNxM + matNxM.
**
** @NOTES: This logic assumes all values are independently operated on, e.g.,
**  m[2][2] = m1[2][2] + m2[2][2]. Therefore, the mat4xM matrix operation can be
**  used.
*/
#define MATRIX_OPERATION(L, F, res, m, p2)                         \
  LUA_MLM_BEGIN                                                    \
  const glmMatrix &m2 = glm_mvalue((p2));                          \
  switch (m.size) {                                                \
    case 2: {                                                      \
      switch (m.secondary) {                                       \
        case 2: glm_newmvalue(L, res, F(m.m22, m2.m22)); return 1; \
        case 3: glm_newmvalue(L, res, F(m.m23, m2.m23)); return 1; \
        case 4: glm_newmvalue(L, res, F(m.m24, m2.m24)); return 1; \
        default:                                                   \
          break;                                                   \
      }                                                            \
      break;                                                       \
    }                                                              \
    case 3: {                                                      \
      switch (m.secondary) {                                       \
        case 2: glm_newmvalue(L, res, F(m.m32, m2.m32)); return 1; \
        case 3: glm_newmvalue(L, res, F(m.m33, m2.m33)); return 1; \
        case 4: glm_newmvalue(L, res, F(m.m34, m2.m34)); return 1; \
        default:                                                   \
          break;                                                   \
      }                                                            \
      break;                                                       \
    }                                                              \
    case 4: {                                                      \
      switch (m.secondary) {                                       \
        case 2: glm_newmvalue(L, res, F(m.m42, m2.m42)); return 1; \
        case 3: glm_newmvalue(L, res, F(m.m43, m2.m43)); return 1; \
        case 4: glm_newmvalue(L, res, F(m.m44, m2.m44)); return 1; \
        default:                                                   \
          break;                                                   \
      }                                                            \
      break;                                                       \
    }                                                              \
    default:                                                       \
      break;                                                       \
  }                                                                \
  LUA_MLM_END

/* F(value, matNxM): e.g., 3 * matNxM */
#define SCALAR_MATRIX_OPERATION(L, F, res, s, p2)             \
  LUA_MLM_BEGIN                                               \
  const glmMatrix &m = glm_mvalue((p2));                      \
  switch (m.size) {                                           \
    case 2: {                                                 \
      switch (m.secondary) {                                  \
        case 2: glm_newmvalue(L, res, F(s, m.m22)); return 1; \
        case 3: glm_newmvalue(L, res, F(s, m.m23)); return 1; \
        case 4: glm_newmvalue(L, res, F(s, m.m24)); return 1; \
        default:                                              \
          break;                                              \
      }                                                       \
      break;                                                  \
    }                                                         \
    case 3: {                                                 \
      switch (m.secondary) {                                  \
        case 2: glm_newmvalue(L, res, F(s, m.m32)); return 1; \
        case 3: glm_newmvalue(L, res, F(s, m.m33)); return 1; \
        case 4: glm_newmvalue(L, res, F(s, m.m34)); return 1; \
        default:                                              \
          break;                                              \
      }                                                       \
      break;                                                  \
    }                                                         \
    case 4: {                                                 \
      switch (m.secondary) {                                  \
        case 2: glm_newmvalue(L, res, F(s, m.m42)); return 1; \
        case 3: glm_newmvalue(L, res, F(s, m.m43)); return 1; \
        case 4: glm_newmvalue(L, res, F(s, m.m44)); return 1; \
        default:                                              \
          break;                                              \
      }                                                       \
      break;                                                  \
    }                                                         \
    default:                                                  \
      break;                                                  \
  }                                                           \
  LUA_MLM_END

/* F(matNxM, value): e.g., matNxM * 3 */
#define MATRIX_SCALAR_OPERATION(L, F, res, m, p2)             \
  LUA_MLM_BEGIN                                               \
  const glm_Float s = SCALAR(p2);                             \
  switch (m.size) {                                           \
    case 2: {                                                 \
      switch (m.secondary) {                                  \
        case 2: glm_newmvalue(L, res, F(m.m22, s)); return 1; \
        case 3: glm_newmvalue(L, res, F(m.m23, s)); return 1; \
        case 4: glm_newmvalue(L, res, F(m.m24, s)); return 1; \
        default:                                              \
          break;                                              \
      }                                                       \
      break;                                                  \
    }                                                         \
    case 3: {                                                 \
      switch (m.secondary) {                                  \
        case 2: glm_newmvalue(L, res, F(m.m32, s)); return 1; \
        case 3: glm_newmvalue(L, res, F(m.m33, s)); return 1; \
        case 4: glm_newmvalue(L, res, F(m.m34, s)); return 1; \
        default:                                              \
          break;                                              \
      }                                                       \
      break;                                                  \
    }                                                         \
    case 4: {                                                 \
      switch (m.secondary) {                                  \
        case 2: glm_newmvalue(L, res, F(m.m42, s)); return 1; \
        case 3: glm_newmvalue(L, res, F(m.m43, s)); return 1; \
        case 4: glm_newmvalue(L, res, F(m.m44, s)); return 1; \
        default:                                              \
          break;                                              \
      }                                                       \
      break;                                                  \
    }                                                         \
    default:                                                  \
      break;                                                  \
  }                                                           \
  LUA_MLM_END

#define MATRIX_DIMS(A, B) m##A##B
#define MATRIX_MULTIPLICATION_OPERATION(L, res, m1, m2, C, R)                               \
  LUA_MLM_BEGIN                                                                             \
  switch (m2.size) {                                                                        \
    case 2: glm_newmvalue(L, res, (m1.MATRIX_DIMS(C, R) * m2.MATRIX_DIMS(2, C))); return 1; \
    case 3: glm_newmvalue(L, res, (m1.MATRIX_DIMS(C, R) * m2.MATRIX_DIMS(3, C))); return 1; \
    case 4: glm_newmvalue(L, res, (m1.MATRIX_DIMS(C, R) * m2.MATRIX_DIMS(4, C))); return 1; \
    default:                                                                                \
      break;                                                                                \
  }                                                                                         \
  LUA_MLM_END

static int num_trybinTM(lua_State *L, const TValue *p1, const TValue *p2, StkId res, TMS event) {
  const glm_Float s = SCALAR(p1);
  switch (event) {
    case TM_ADD: {
      switch (ttypetag(p2)) {
        case LUA_VVECTOR2: glm_setvvalue2s(res, s + glm_vecvalue(p2).v2, LUA_VVECTOR2); return 1;
        case LUA_VVECTOR3: glm_setvvalue2s(res, s + glm_vecvalue(p2).v3, LUA_VVECTOR3); return 1;
        case LUA_VVECTOR4: glm_setvvalue2s(res, s + glm_vecvalue(p2).v4, LUA_VVECTOR4); return 1;
        default:
          break;
      }
      break;
    }
    case TM_SUB: {
      switch (ttypetag(p2)) {
        case LUA_VVECTOR2: glm_setvvalue2s(res, s - glm_vecvalue(p2).v2, LUA_VVECTOR2); return 1;
        case LUA_VVECTOR3: glm_setvvalue2s(res, s - glm_vecvalue(p2).v3, LUA_VVECTOR3); return 1;
        case LUA_VVECTOR4: glm_setvvalue2s(res, s - glm_vecvalue(p2).v4, LUA_VVECTOR4); return 1;
        default:
          break;
      }
      break;
    }
    case TM_MUL: {
      switch (ttypetag(p2)) {
        case LUA_VVECTOR2: glm_setvvalue2s(res, s * glm_vecvalue(p2).v2, LUA_VVECTOR2); return 1;
        case LUA_VVECTOR3: glm_setvvalue2s(res, s * glm_vecvalue(p2).v3, LUA_VVECTOR3); return 1;
        case LUA_VVECTOR4: glm_setvvalue2s(res, s * glm_vecvalue(p2).v4, LUA_VVECTOR4); return 1;
        case LUA_VQUAT: glm_setvvalue2s(res, s * glm_quatvalue(p2).q, LUA_VQUAT); return 1;
        case LUA_VMATRIX: SCALAR_MATRIX_OPERATION(L, operator*, res, s, p2); break;
        default:
          break;
      }
      break;
    }
    case TM_DIV: {
      switch (ttypetag(p2)) {
        case LUA_VVECTOR2: glm_setvvalue2s(res, s / glm_vecvalue(p2).v2, LUA_VVECTOR2); return 1;
        case LUA_VVECTOR3: glm_setvvalue2s(res, s / glm_vecvalue(p2).v3, LUA_VVECTOR3); return 1;
        case LUA_VVECTOR4: glm_setvvalue2s(res, s / glm_vecvalue(p2).v4, LUA_VVECTOR4); return 1;
        case LUA_VQUAT: glm_setvvalue2s(res, s * glm::inverse(glm_quatvalue(p2).q), LUA_VQUAT); return 1;
        case LUA_VMATRIX: SCALAR_MATRIX_OPERATION(L, operator/, res, s, p2); break;
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
  return 0;
}

static int vec_trybinTM(lua_State *L, const TValue *p1, const TValue *p2, StkId res, TMS event) {
  UNUSED(L);
  const glmVector &v = glm_vvalue(p1);
  const int tt_p1 = ttypetag(p1), tt_p2 = ttypetag(p2);
  switch (event) {
    case TM_ADD: {
      if (tt_p1 == tt_p2)
        VECTOR_OPERATION(operator+, res, v, p2);
      if (tt_p2 == LUA_VNUMINT || tt_p2 == LUA_VNUMFLT)
        VECTOR_SCALAR_OPERATION(operator+, res, v, p2);
      break;
    }
    case TM_SUB: {
      if (tt_p1 == tt_p2)
        VECTOR_OPERATION(operator-, res, v, p2);
      if (tt_p2 == LUA_VNUMINT || tt_p2 == LUA_VNUMFLT)
        VECTOR_SCALAR_OPERATION(operator-, res, v, p2);
      break;
    }
    case TM_MUL: {
      if (tt_p1 == tt_p2)
        VECTOR_OPERATION(operator*, res, v, p2);
      if (tt_p2 == LUA_VNUMINT || tt_p2 == LUA_VNUMFLT)
        VECTOR_SCALAR_OPERATION(operator*, res, v, p2);
      if (tt_p2 == LUA_VQUAT) {
        switch (tt_p1) {
          case LUA_VVECTOR3: glm_setvvalue2s(res, v.v3 * glm_quatvalue(p2).q, LUA_VVECTOR3); return 1;
          case LUA_VVECTOR4: glm_setvvalue2s(res, v.v4 * glm_quatvalue(p2).q, LUA_VVECTOR4); return 1;
          default:
            break;
        }
      }
      else if (tt_p2 == LUA_VMATRIX && mvalue(p2).secondary == glm_dimensions(tt_p1)) {
        const glmMatrix &m2 = glm_mvalue(p2);
        switch (m2.size) {
          case 2: {
            switch (m2.secondary) {
              case 2: glm_setvvalue2s(res, v.v2 * m2.m22, LUA_VVECTOR2); return 1;
              case 3: glm_setvvalue2s(res, v.v3 * m2.m23, LUA_VVECTOR2); return 1;
              case 4: glm_setvvalue2s(res, v.v4 * m2.m24, LUA_VVECTOR2); return 1;
              default:
                break;
            }
            break;
          }
          case 3: {
            switch (m2.secondary) {
              case 2: glm_setvvalue2s(res, v.v2 * m2.m32, LUA_VVECTOR3); return 1;
              case 3: glm_setvvalue2s(res, v.v3 * m2.m33, LUA_VVECTOR3); return 1;
              case 4: glm_setvvalue2s(res, v.v4 * m2.m34, LUA_VVECTOR3); return 1;
              default:
                break;
            }
            break;
          }
          case 4: {
            switch (m2.secondary) {
              case 2: glm_setvvalue2s(res, v.v2 * m2.m42, LUA_VVECTOR4); return 1;
              case 3: glm_setvvalue2s(res, v.v3 * m2.m43, LUA_VVECTOR4); return 1;
              case 4: glm_setvvalue2s(res, v.v4 * m2.m44, LUA_VVECTOR4); return 1;
              default:
                break;
            }
            break;
          }
          default:
            break;
        }
      }
      // Rotating a point by only the rotation part of a transformation matrix.
      else if (tt_p1 == LUA_VVECTOR3 && tt_p2 == LUA_VMATRIX) {
        const glmMatrix &m2 = glm_mvalue(p2);
        if (tt_p1 == LUA_VVECTOR3 && tt_p2 == LUA_VMATRIX && m2.size == 4 && m2.secondary == 4) {
          glm_setvvalue2s(res, (glm::vec<4, glm_Float>(v.v3, glm_Float(1)) * m2.m44), LUA_VVECTOR4);
          return 1;
        }
      }
      break;
    }
    case TM_DIV: {
      if (tt_p1 == tt_p2)
        VECTOR_OPERATION(operator/, res, v, p2);
      if (tt_p2 == LUA_VNUMINT || tt_p2 == LUA_VNUMFLT)
        VECTOR_SCALAR_OPERATION(operator/, res, v, p2);
      if (tt_p2 == LUA_VMATRIX) {
        const glmMatrix &m2 = glm_mvalue(p2);
        if (m2.size == m2.secondary && tt_p2 == glm_variant(m2.size)) {
          switch (m2.size) {
            case LUA_VVECTOR2: glm_setvvalue2s(res, v.v2 / m2.m22, LUA_VVECTOR2); return 1;
            case LUA_VVECTOR3: glm_setvvalue2s(res, v.v3 / m2.m33, LUA_VVECTOR3); return 1;
            case LUA_VVECTOR4: glm_setvvalue2s(res, v.v4 / m2.m44, LUA_VVECTOR4); return 1;
            default:
              break;
          }
          break;
        }
      }
      break;
    }
    case TM_IDIV: {
      if (tt_p1 == tt_p2)
        VECTOR_OPERATION(glm::__objFloorDivide, res, v, p2);
      if (tt_p2 == LUA_VNUMINT || tt_p2 == LUA_VNUMFLT)
        VECTOR_SCALAR_OPERATION(glm::__objFloorDivide, res, v, p2);
      break;
    }
    case TM_MOD: {  // Using fmod for the same reasons described in llimits.h
      if (tt_p1 == tt_p2)
        VECTOR_OPERATION(glm::fmod, res, v, p2);
      if (tt_p2 == LUA_VNUMINT || tt_p2 == LUA_VNUMFLT)
        VECTOR_SCALAR_OPERATION(glm::fmod, res, v, p2);
      break;
    }
    case TM_POW: {
      if (tt_p1 == tt_p2)
        VECTOR_OPERATION(glm::pow, res, v, p2);
      if (tt_p2 == LUA_VNUMINT || tt_p2 == LUA_VNUMFLT)
        VECTOR_SCALAR_OPERATION(glm::__objPow, res, v, p2);
      break;
    }
    case TM_UNM: {
      switch (tt_p1) {
        case LUA_VVECTOR2: glm_setvvalue2s(res, -v.v2, LUA_VVECTOR2); return 1;
        case LUA_VVECTOR3: glm_setvvalue2s(res, -v.v3, LUA_VVECTOR3); return 1;
        case LUA_VVECTOR4: glm_setvvalue2s(res, -v.v4, LUA_VVECTOR4); return 1;
        default:
          break;
      }
      break;
    }
    case TM_BAND: INT_VECTOR_OPERATION(operator&, res, v, p2); break;
    case TM_BOR: INT_VECTOR_OPERATION(operator|, res, v, p2); break;
    case TM_BXOR: INT_VECTOR_OPERATION(operator^, res, v, p2); break;
    case TM_SHL: INT_VECTOR_OPERATION(operator<<, res, v, p2); break;
    case TM_SHR: INT_VECTOR_OPERATION(operator>>, res, v, p2); break;
    default:
      break;
  }
  return 0;
}

static int quat_trybinTM(lua_State *L, const TValue *p1, const TValue *p2, StkId res, TMS event) {
  UNUSED(L);
  const glmVector &v = glm_vvalue(p1);
  const int tt_p2 = ttypetag(p2);
  switch (event) {
    case TM_UNM: glm_setvvalue2s(res, -v.q, LUA_VQUAT); return 1;
    case TM_ADD: {
      if (tt_p2 == LUA_VQUAT) {
        glm_setvvalue2s(res, glm_quatvalue(p1).q + glm_quatvalue(p2).q, LUA_VQUAT);
        return 1;
      }
      break;
    }
    case TM_SUB: {
      if (tt_p2 == LUA_VQUAT) {
        glm_setvvalue2s(res, glm_quatvalue(p1).q - glm_quatvalue(p2).q, LUA_VQUAT);
        return 1;
      }
      break;
    }
    case TM_POW: {
      if (tt_p2 == LUA_VNUMINT || tt_p2 == LUA_VNUMFLT) {
        glm_setvvalue2s(res, glm::pow(v.q, SCALAR(p2)), LUA_VQUAT);
        return 1;
      }
      break;
    }
    case TM_MUL: {
      switch (tt_p2) {
        case LUA_VNUMINT: glm_setvvalue2s(res, v.q * cast_glmfloat(ivalue(p2)), LUA_VQUAT); return 1;
        case LUA_VNUMFLT: glm_setvvalue2s(res, v.q * cast_glmfloat(fltvalue(p2)), LUA_VQUAT); return 1;
        case LUA_VVECTOR3: glm_setvvalue2s(res, v.q * glm_vecvalue(p2).v3, LUA_VVECTOR3); return 1;
        case LUA_VVECTOR4: glm_setvvalue2s(res, v.q * glm_vecvalue(p2).v4, LUA_VVECTOR4); return 1;
        case LUA_VQUAT: glm_setvvalue2s(res, v.q * glm_quatvalue(p2).q, LUA_VQUAT); return 1;
        default:
          break;
      }
      break;
    }
    case TM_DIV: {
      if (tt_p2 == LUA_VNUMINT || tt_p2 == LUA_VNUMFLT) {
        const glm_Float s = SCALAR(p2);
        glm::qua<glm_Float> result = glm::identity<glm::qua<glm_Float>>();
        if (glm::notEqual(s, cast_glmfloat(0), glm::epsilon<glm_Float>()))
          result = v.q / s;

        glm_setvvalue2s(res, result, LUA_VQUAT);
        return 1;
      }
      break;
    }
    default:
      break;
  }
  return 0;
}

static int mat_trybinTM(lua_State *L, const TValue *p1, const TValue *p2, StkId res, TMS event) {
  const glmMatrix &m = glm_mvalue(p1);
  const int tt_p2 = ttypetag(p2);
  switch (event) {
    case TM_ADD: {
      if (tt_p2 == LUA_VMATRIX && m.size == mvalue(p2).size && m.secondary == mvalue(p2).secondary)
        MATRIX_OPERATION(L, operator+, res, m, p2);
      if (tt_p2 == LUA_VNUMINT || tt_p2 == LUA_VNUMFLT)
        MATRIX_SCALAR_OPERATION(L, operator+, res, m, p2);
      break;
    }
    case TM_SUB: {
      if (tt_p2 == LUA_VMATRIX && m.size == mvalue(p2).size && m.secondary == mvalue(p2).secondary)
        MATRIX_OPERATION(L, operator-, res, m, p2);
      if (tt_p2 == LUA_VNUMINT || tt_p2 == LUA_VNUMFLT)
        MATRIX_SCALAR_OPERATION(L, operator-, res, m, p2);
      break;
    }
    case TM_MUL: {
      if (tt_p2 == LUA_VNUMINT || tt_p2 == LUA_VNUMFLT)
        MATRIX_SCALAR_OPERATION(L, operator*, res, m, p2);
      if (tt_p2 == LUA_VVECTOR3 && m.size == 4 && m.secondary == 4) {
        const typename glm::mat<4, 4, glm_Float>::col_type &r =
#if defined(GLM_LUA_ROTATE_DIRECTION)
          // Transforms the given direction vector by this matrix m: M * (x, y, z, 0).
          m.m44 * glm::vec<4, glm_Float>(glm_vvalue(p2).v3, glm_Float(0));
#else
          // Transforms the given point vector by this matrix M: M * (x, y, z, 1).
          m.m44 * glm::vec<4, glm_Float>(glm_vvalue(p2).v3, glm_Float(1));
#endif
        glm_setvvalue2s(res, (glm::vec<3, glm_Float>(r.x, r.y, r.z)), LUA_VVECTOR3);
        return 1;
      }
	    else if (tt_p2 == LUA_VMATRIX && m.size == mvalue(p2).secondary) {
        const glmMatrix &m2 = glm_mvalue(p2);
        switch (m.size) {
          case 2: {
            switch (m.secondary) {
              case 2: MATRIX_MULTIPLICATION_OPERATION(L, res, m, m2, 2, 2); break;
              case 3: MATRIX_MULTIPLICATION_OPERATION(L, res, m, m2, 2, 3); break;
              case 4: MATRIX_MULTIPLICATION_OPERATION(L, res, m, m2, 2, 4); break;
              default:
                break;
            }
            break;
          }
          case 3: {
            switch (m.secondary) {
              case 2: MATRIX_MULTIPLICATION_OPERATION(L, res, m, m2, 3, 2); break;
              case 3: MATRIX_MULTIPLICATION_OPERATION(L, res, m, m2, 3, 3); break;
              case 4: MATRIX_MULTIPLICATION_OPERATION(L, res, m, m2, 3, 4); break;
              default:
                break;
            }
            break;
          }
          case 4: {
            switch (m.secondary) {
              case 2: MATRIX_MULTIPLICATION_OPERATION(L, res, m, m2, 4, 2); break;
              case 3: MATRIX_MULTIPLICATION_OPERATION(L, res, m, m2, 4, 3); break;
              case 4: MATRIX_MULTIPLICATION_OPERATION(L, res, m, m2, 4, 4); break;
              default:
                break;
            }
            break;
          }
          default:
            break;
        }
      }
      else if (tt_p2 == glm_variant(m.size)) {
        const glmVector &v2 = glm_vvalue(p2);
        switch (m.size) {
          case 2: {
            switch (m.secondary) {
              case 2: glm_setvvalue2s(res, m.m22 * v2.v2, LUA_VVECTOR2); return 1;
              case 3: glm_setvvalue2s(res, m.m23 * v2.v2, LUA_VVECTOR3); return 1;
              case 4: glm_setvvalue2s(res, m.m24 * v2.v2, LUA_VVECTOR4); return 1;
              default:
                break;
            }
            break;
          }
          case 3: {
            switch (m.secondary) {
              case 2: glm_setvvalue2s(res, m.m32 * v2.v3, LUA_VVECTOR2); return 1;
              case 3: glm_setvvalue2s(res, m.m33 * v2.v3, LUA_VVECTOR3); return 1;
              case 4: glm_setvvalue2s(res, m.m34 * v2.v3, LUA_VVECTOR4); return 1;
              default:
                break;
            }
            break;
          }
          case 4: {
            switch (m.secondary) {
              case 2: glm_setvvalue2s(res, m.m42 * v2.v4, LUA_VVECTOR2); return 1;
              case 3: glm_setvvalue2s(res, m.m43 * v2.v4, LUA_VVECTOR3); return 1;
              case 4: glm_setvvalue2s(res, m.m44 * v2.v4, LUA_VVECTOR4); return 1;
              default:
                break;
            }
            break;
          }
          default:
            break;
        }
      }
      break;
    }
    case TM_DIV: {
      if (m.size == m.secondary && tt_p2 == LUA_VMATRIX) {  // operator/(matNxN, matNxN)
        const glmMatrix &m2 = glm_mvalue(p2);
        if (m.size == m2.size && m2.size == m2.secondary) {
          switch (m.size) {
            case 2: glm_newmvalue(L, res, m.m22 / m2.m22); return 1;
            case 3: glm_newmvalue(L, res, m.m33 / m2.m33); return 1;
            case 4: glm_newmvalue(L, res, m.m44 / m2.m44); return 1;
            default:
              break;
          }
        }
      }
      else if (tt_p2 == glm_variant(m.size)) {  // operator/(matrix, vector)
        const glmVector &v2 = glm_vvalue(p2);
        switch (m.size) {
          case 2: glm_setvvalue2s(res, m.m22 / v2.v2, LUA_VVECTOR2); return 1;
          case 3: glm_setvvalue2s(res, m.m33 / v2.v3, LUA_VVECTOR3); return 1;
          case 4: glm_setvvalue2s(res, m.m44 / v2.v4, LUA_VVECTOR4); return 1;
          default:
            break;
        }
      }
      else if (tt_p2 == LUA_VNUMINT || tt_p2 == LUA_VNUMFLT)
        MATRIX_SCALAR_OPERATION(L, operator/, res, m, p2);
      break;
    }
    case TM_UNM: {
      switch (m.size) {
        case 2: {
          switch (m.secondary) {
            case 2: glm_newmvalue(L, res, -m.m22); return 1;
            case 3: glm_newmvalue(L, res, -m.m23); return 1;
            case 4: glm_newmvalue(L, res, -m.m24); return 1;
            default:
              break;
          }
          break;
        }
        case 3: {
          switch (m.secondary) {
            case 2: glm_newmvalue(L, res, -m.m32); return 1;
            case 3: glm_newmvalue(L, res, -m.m33); return 1;
            case 4: glm_newmvalue(L, res, -m.m34); return 1;
            default:
              break;
          }
          break;
        }
        case 4: {
          switch (m.secondary) {
            case 2: glm_newmvalue(L, res, -m.m42); return 1;
            case 3: glm_newmvalue(L, res, -m.m43); return 1;
            case 4: glm_newmvalue(L, res, -m.m44); return 1;
            default:
              break;
          }
          break;
        }
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
  return 0;
}

/* }================================================================== */