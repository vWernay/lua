/*
** $Id: lgrit_math.c $
**
** Vector math operations for lua vectors. Note, much of this API has been
** DEPRECATED by the glm binding library.
**
** See Copyright Notice in lua.h
*/
#define lgrit_math_c
#define LUA_LIB

#include <math.h>

#include "lua.h"
#include "luaconf.h"
#include "lauxlib.h"
#include "ldebug.h"
#include "llimits.h"
#include "lgrit.h"
#include "lgrit_lib.h"

#define luai_numlt_c(a,b) luai_numlt(a, b) ? (a) : (b)
#define luai_numle_c(a,b) luai_numle(a, b) ? (a) : (b)
#define luai_numgt_c(a,b) luai_numgt(a, b) ? (a) : (b)
#define luai_numge_c(a,b) luai_numge(a, b) ? (a) : (b)

#define ADDF(x, y) ((x) + (y))
#define SUBF(x, y) ((x) - (y))
#define MULF(x, y) ((x) * (y))
#define DIVF(x, y) ((x) / (y))

#define OP1(f, lhs, r) LUA_MLM_BEGIN r.x = f(lhs.x); LUA_MLM_END
#define OP2(f, lhs, r) LUA_MLM_BEGIN r.x = f(lhs.x); r.y = f(lhs.y); LUA_MLM_END
#define OP3(f, lhs, r) LUA_MLM_BEGIN r.x = f(lhs.x); r.y = f(lhs.y); r.z = f(lhs.z); LUA_MLM_END
#define OP4(f, lhs, r) LUA_MLM_BEGIN r.x = f(lhs.x); r.y = f(lhs.y); r.z = f(lhs.z);  r.w = f(lhs.w); LUA_MLM_END

#define PW1(f, lhs, rhs, r) LUA_MLM_BEGIN r.x = f(lhs.x, rhs.x); LUA_MLM_END
#define PW2(f, lhs, rhs, r) LUA_MLM_BEGIN r.x = f(lhs.x, rhs.x); r.y = f(lhs.y, rhs.y); LUA_MLM_END
#define PW3(f, lhs, rhs, r) LUA_MLM_BEGIN r.x = f(lhs.x, rhs.x); r.y = f(lhs.y, rhs.y); r.z = f(lhs.z, rhs.z); LUA_MLM_END
#define PW4(f, lhs, rhs, r) LUA_MLM_BEGIN r.x = f(lhs.x, rhs.x); r.y = f(lhs.y, rhs.y); r.z = f(lhs.z, rhs.z);  r.w = f(lhs.w, rhs.w); LUA_MLM_END

#define SCALAR1(f, lhs, rhs, r) LUA_MLM_BEGIN r.x = f(lhs.x, rhs); LUA_MLM_END
#define SCALAR2(f, lhs, rhs, r) LUA_MLM_BEGIN r.x = f(lhs.x, rhs); r.y = f(lhs.y, rhs); LUA_MLM_END
#define SCALAR3(f, lhs, rhs, r) LUA_MLM_BEGIN r.x = f(lhs.x, rhs); r.y = f(lhs.y, rhs); r.z = f(lhs.z, rhs); LUA_MLM_END
#define SCALAR4(f, lhs, rhs, r) LUA_MLM_BEGIN r.x = f(lhs.x, rhs); r.y = f(lhs.y, rhs); r.z = f(lhs.z, rhs);  r.w = f(lhs.w, rhs); LUA_MLM_END
#define SCALAR2B(f, lhs, rhs, r) LUA_MLM_BEGIN r.x = f(rhs, lhs.x); r.y = f(rhs, lhs.y); LUA_MLM_END
#define SCALAR3B(f, lhs, rhs, r) LUA_MLM_BEGIN r.x = f(rhs, lhs.x); r.y = f(rhs, lhs.y); r.z = f(rhs, lhs.z); LUA_MLM_END
#define SCALAR4B(f, lhs, rhs, r) LUA_MLM_BEGIN r.x = f(rhs, lhs.x); r.y = f(rhs, lhs.y); r.z = f(rhs, lhs.z);  r.w = f(rhs, lhs.w); LUA_MLM_END

#define DOT2(c, lhs, rhs) c((lhs.x * rhs.x) + (lhs.y * rhs.y))
#define DOT3(c, lhs, rhs) c((lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z))
#define DOT4(c, lhs, rhs) c((lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w))

#define LUA_VEC_OP_UNARY(L, op)                                 \
  LUA_MLM_BEGIN                                                 \
  int variant;                                                  \
  lua_Float4 v;                                                 \
  switch ((variant = lua_tovector((L), 1, V_PARSETABLE, &v))) { \
    case LUA_VVECTOR1: OP1(l_vecop(op), v, v); break;           \
    case LUA_VVECTOR2: OP2(l_vecop(op), v, v); break;           \
    case LUA_VVECTOR3: OP3(l_vecop(op), v, v); break;           \
    case LUA_VVECTOR4: OP4(l_vecop(op), v, v); break;           \
    default: return luaL_typeerror(L, 1, LABEL_ALL);            \
  }                                                             \
  lua_pushvector((L), v, variant);                              \
  return 1;                                                     \
  LUA_MLM_END

#define LUA_VEC_OP_BINARY(L, op)                              \
  LUA_MLM_BEGIN                                               \
  int variant;                                                \
  lua_VecF x;                                                 \
  lua_Float4 v, v2;                                           \
  switch ((variant = lua_tovector(L, 1, V_PARSETABLE, &v))) { \
    case LUA_VVECTOR1:                                        \
      x = cast_vec(luaL_checknumber(L, 2));                   \
      SCALAR1(l_vecop(op), v, x, v);                          \
      break;                                                  \
    case LUA_VVECTOR2:                                        \
      if (lua_type(L, 2) == LUA_TNUMBER) {                    \
        x = cast_vec(luaL_checknumber(L, 2));                 \
        SCALAR2(l_vecop(op), v, x, v);                        \
      }                                                       \
      else if (lua_type(L, 2) == LUA_TVECTOR) {               \
        lua_checkv2(L, 2, V_PARSETABLE, &v2);                 \
        PW2(l_vecop(op), v, v2, v);                           \
      }                                                       \
      else                                                    \
        return luaL_typeerror(L, 2, LABEL_VECTOR2);           \
      break;                                                  \
    case LUA_VVECTOR3:                                        \
      if (lua_type(L, 2) == LUA_TNUMBER) {                    \
        x = cast_vec(luaL_checknumber(L, 2));                 \
        SCALAR3(l_vecop(op), v, x, v);                        \
      }                                                       \
      else if (lua_type(L, 2) == LUA_TVECTOR) {               \
        lua_checkv3(L, 2, V_PARSETABLE, &v2);                 \
        PW3(l_vecop(op), v, v2, v);                           \
      }                                                       \
      else                                                    \
        return luaL_typeerror(L, 2, LABEL_VECTOR3);           \
      break;                                                  \
    case LUA_VVECTOR4:                                        \
      if (lua_type(L, 2) == LUA_TNUMBER) {                    \
        x = cast_vec(luaL_checknumber(L, 2));                 \
        SCALAR3(l_vecop(op), v, x, v);                        \
      }                                                       \
      else if (lua_type(L, 2) == LUA_TVECTOR) {               \
        lua_checkv4(L, 2, V_PARSETABLE, &v2);                 \
        PW4(l_vecop(op), v, v2, v);                           \
      }                                                       \
      else                                                    \
        return luaL_typeerror(L, 2, LABEL_VECTOR4);           \
      break;                                                  \
    default:                                                  \
      return luaL_typeerror(L, 1, LABEL_ALL);                 \
  }                                                           \
  lua_pushvector(L, v, variant);                              \
  return 1;                                                   \
  LUA_MLM_END

#define LUA_VEC_CAND(L, op)                                     \
  LUA_MLM_BEGIN                                                 \
  lua_Float4 v;                                                 \
  int variant, result = 1;                                      \
  switch ((variant = lua_tovector((L), 1, V_PARSETABLE, &v))) { \
    case LUA_VVECTOR4: result &= op(v.w); LUA_FALLTHROUGH;      \
    case LUA_VVECTOR3: result &= op(v.z); LUA_FALLTHROUGH;      \
    case LUA_VVECTOR2: result &= op(v.y); LUA_FALLTHROUGH;      \
    case LUA_VVECTOR1: result &= op(v.x); break;                \
    default: return luaL_typeerror(L, 1, LABEL_ALL);            \
  }                                                             \
  lua_pushboolean((L), result);                                 \
  return 1;                                                     \
  LUA_MLM_END

#define LUA_VEC_COR(L, op)                                      \
  LUA_MLM_BEGIN                                                 \
  lua_Float4 v;                                                 \
  int variant, result = 0;                                      \
  switch ((variant = lua_tovector((L), 1, V_PARSETABLE, &v))) { \
    case LUA_VVECTOR4: result |= op(v.w); LUA_FALLTHROUGH;      \
    case LUA_VVECTOR3: result |= op(v.z); LUA_FALLTHROUGH;      \
    case LUA_VVECTOR2: result |= op(v.y); LUA_FALLTHROUGH;      \
    case LUA_VVECTOR1: result |= op(v.x); break;                \
    default: return luaL_typeerror(L, 1, LABEL_ALL);            \
  }                                                             \
  lua_pushboolean((L), result);                                 \
  return 1;                                                     \
  LUA_MLM_END

static LUA_INLINE float todegf(float x) { return x * (180.f / 3.141592653589793238462643383279502884f); }
static LUA_INLINE float toradf(float x) { return x * (3.141592653589793238462643383279502884f / 180.f); }

static LUA_INLINE double todeg(double x) { return x * (180.0 / 3.141592653589793238462643383279502884); }
#if LUA_VEC_TYPE == LUA_FLOAT_DOUBLE
static LUA_INLINE double torad(double x) { return x * (3.141592653589793238462643383279502884 / 180.0); }
#endif

#if LUA_VEC_TYPE == LUA_FLOAT_LONGDOUBLE
static LUA_INLINE long double todegl(long double x) { return x * (180.0L / 3.141592653589793238462643383279502884L); }
static LUA_INLINE long double toradl(long double x) { return x * (3.141592653589793238462643383279502884L / 180.0L); }
#endif

#ifdef _MSC_VER
__pragma(warning(push))
__pragma(warning(disable : 26451))
#endif

/*
** {==================================================================
** Tag Methods
** ===================================================================
*/

/* Handle l_noret & unreachable code warnings */
#define ERR_DIVZERO(L) luaG_runerror(L, "division by zero")
#define ERR_INVALID_OP "Cannot use that op with %s and %s"

int luaVec_trybinTM (lua_State *L, const TValue *p1, const TValue *p2, StkId res, TMS event) {
  lua_Float4 nb, nc;
  lua_Float4 r = V_ZEROVEC;
  const int nb_count = luaVec_parse(L, p1, &nb);
  const int nc_count = luaVec_parse(L, p2, &nc);

  int result = 1;
  if (nb_count == 3 && nc_count == 3) {  /* <vec3, op, vec3> */
    switch (event) {
      case TM_ADD: PW3(ADDF, nb, nc, r); break;
      case TM_SUB: PW3(SUBF, nb, nc, r); break;
      case TM_MUL: PW3(MULF, nb, nc, r); break;
      case TM_MOD: PW3(l_vecop(fmod), nb, nc, r); break;
      case TM_POW: PW3(l_vecop(pow), nb, nc, r); break;
      case TM_UNM: r.x = -nb.x; r.y = -nb.y; r.z = -nb.z; break;
      case TM_DIV:
        if (V_ISZERO(nc.x) || V_ISZERO(nc.y) || V_ISZERO(nc.z)) {
          ERR_DIVZERO(L);
        }

        PW3(DIVF, nb, nc, r);
        break;
      case TM_IDIV:
        if (V_ISZERO(nc.x) || V_ISZERO(nc.y) || V_ISZERO(nc.z)) {
          ERR_DIVZERO(L);
        }

        PW3(DIVF, nb, nc, r);
        OP3(l_vecop(floor), r, r);
        break;
      default:
        luaG_runerror(L, ERR_INVALID_OP, LABEL_VECTOR3, LABEL_VECTOR3);
    }
    setvvalue(s2v(res), r, LUA_VVECTOR3);
  }
  else if (ttisquat(p1) && nc_count == 3) {  /* <quat, op, vec3> */
    switch (event) {
      case TM_MUL: {
        const lua_VecF a = nb.w, b = nb.x, c = nb.y, d = nb.z;
        const lua_VecF mat[3][3] = { /* row major */
          { a*a + b*b - c*c - d*d, 2*b*c - 2*a*d, 2*b*d + 2*a*c },
          { 2*b*c + 2*a*d, a*a - b*b + c*c - d*d, 2*c*d - 2*a*b },
          { 2*b*d - 2*a*c, 2*c*d + 2*a*b, a*a - b*b - c*c + d*d },
        };

        r.x = mat[0][0] * nc.x + mat[0][1] * nc.y + mat[0][2] * nc.z;
        r.y = mat[1][0] * nc.x + mat[1][1] * nc.y + mat[1][2] * nc.z;
        r.z = mat[2][0] * nc.x + mat[2][1] * nc.y + mat[2][2] * nc.z;
        break;
      }
      default:
        luaG_runerror(L, ERR_INVALID_OP, LABEL_QUATERN, LABEL_VECTOR3);
    }
    setvvalue(s2v(res), r, LUA_VVECTOR3);
  }
  else if (ttisquat(p1) && ttisquat(p2)) {  /* <quat, op, quat> */
    switch (event) {
      case TM_MUL:
        r.w = nb.w * nc.w - nb.x * nc.x - nb.y * nc.y - nb.z * nc.z;
        r.x = nb.w * nc.x + nb.x * nc.w + nb.y * nc.z - nb.z * nc.y;
        r.y = nb.w * nc.y + nb.y * nc.w + nb.z * nc.x - nb.x * nc.z;
        r.z = nb.w * nc.z + nb.z * nc.w + nb.x * nc.y - nb.y * nc.x;
        break;
      default:
        luaG_runerror(L, ERR_INVALID_OP, LABEL_QUATERN, LABEL_QUATERN);
    }
    setvvalue(s2v(res), r, LUA_VQUAT);
  }
  else if (nb_count == 2 && nc_count == 2) {  /* <vec2, op, vec2> */
    switch (event) {
      case TM_ADD: PW2(ADDF, nb, nc, r); break;
      case TM_SUB: PW2(SUBF, nb, nc, r); break;
      case TM_MUL: PW2(MULF, nb, nc, r); break;
      case TM_MOD: PW2(l_vecop(fmod), nb, nc, r); break;
      case TM_POW: PW2(l_vecop(pow), nb, nc, r); break;
      case TM_UNM: r.x = -nb.x; r.y = -nb.y; break;
      case TM_DIV:
        if (V_ISZERO(nc.x) || V_ISZERO(nc.y)) {
          ERR_DIVZERO(L);
        }

        PW2(DIVF, nb, nc, r);
        break;
      case TM_IDIV:
        if (V_ISZERO(nc.x) || V_ISZERO(nc.y)) {
          ERR_DIVZERO(L);
        }

        PW2(DIVF, nb, nc, r);
        OP2(l_vecop(floor), r, r);
        break;
      default:
        luaG_runerror(L, ERR_INVALID_OP, LABEL_VECTOR2, LABEL_VECTOR2);
    }
    setvvalue(s2v(res), r, LUA_VVECTOR2);
  }
  else if (nb_count == 4 && nc_count == 4) {  /* <vec4, op, vec4> */
    switch (event) {
      case TM_ADD: PW4(ADDF, nb, nc, r); break;
      case TM_SUB: PW4(SUBF, nb, nc, r); break;
      case TM_MUL: PW4(MULF, nb, nc, r); break;
      case TM_MOD: PW4(l_vecop(fmod), nb, nc, r); break;
      case TM_POW: PW4(l_vecop(pow), nb, nc, r); break;
      case TM_UNM: r.x = -nb.x; r.y = -nb.y; r.z = -nb.z; r.w = -nb.w; break;
      case TM_DIV:
        if (V_ISZERO(nc.x) || V_ISZERO(nc.y) || V_ISZERO(nc.z) || V_ISZERO(nc.w)) {
          ERR_DIVZERO(L);
        }

        PW4(DIVF, nb, nc, r);
        break;
      case TM_IDIV:
        if (V_ISZERO(nc.x) || V_ISZERO(nc.y) || V_ISZERO(nc.z)) {
          ERR_DIVZERO(L);
        }

        PW4(DIVF, nb, nc, r);
        OP4(l_vecop(floor), r, r);
        break;
      default:
        luaG_runerror(L, ERR_INVALID_OP, LABEL_VECTOR4, LABEL_VECTOR4);
    }
    setvvalue(s2v(res), r, LUA_VVECTOR4);
  }
  else if (nb_count == 3 && ttisnumber(p2)) {  /* <vec3, op, numeric> */
    lua_VecF nc_v = cast_vec(nvalue(p2));
    switch (event) {
      case TM_ADD: SCALAR3(ADDF, nb, nc_v, r); break;
      case TM_SUB: SCALAR3(SUBF, nb, nc_v, r); break;
      case TM_MUL: SCALAR3(MULF, nb, nc_v, r); break;
      case TM_MOD: SCALAR3(l_vecop(fmod), nb, nc_v, r); break;
      case TM_POW: SCALAR3(l_vecop(pow), nb, nc_v, r); break;
      case TM_DIV:
        if (V_ISZERO(nc_v)) {
          ERR_DIVZERO(L);
        }

        SCALAR3(DIVF, nb, nc_v, r);
        break;
      case TM_IDIV:
        if (V_ISZERO(nc_v)) {
          ERR_DIVZERO(L);
        }

        SCALAR3(DIVF, nb, nc_v, r);
        OP3(l_vecop(floor), r, r);
        break;
      default:
        luaG_runerror(L, ERR_INVALID_OP, LABEL_VECTOR3, LABEL_NUMBER);
    }
    setvvalue(s2v(res), r, LUA_VVECTOR3);
  }
  else if (nb_count == 2 && ttisnumber(p2)) {  /* <vec2, op, numeric> */
    lua_VecF nc_v = cast_vec(nvalue(p2));
    switch (event) {
      case TM_ADD: SCALAR2(ADDF, nb, nc_v, r); break;
      case TM_SUB: SCALAR2(SUBF, nb, nc_v, r); break;
      case TM_MUL: SCALAR2(MULF, nb, nc_v, r); break;
      case TM_MOD: SCALAR2(l_vecop(fmod), nb, nc_v, r); break;
      case TM_POW: SCALAR2(l_vecop(pow), nb, nc_v, r); break;
      case TM_DIV:
        if (V_ISZERO(nc_v)) {
          ERR_DIVZERO(L);
        }

        SCALAR2(DIVF, nb, nc_v, r);
        break;
      case TM_IDIV:
        if (V_ISZERO(nc_v)) {
          ERR_DIVZERO(L);
        }

        SCALAR2(DIVF, nb, nc_v, r);
        OP2(l_vecop(floor), r, r);
        break;
      default:
        luaG_runerror(L, ERR_INVALID_OP, LABEL_VECTOR2, LABEL_NUMBER);
    }
    setvvalue(s2v(res), r, LUA_VVECTOR2);
  }
  else if (nb_count == 4 && ttisnumber(p2)) {  /* <vec4, op, numeric> */
    lua_VecF nc_v = cast_vec(nvalue(p2));
    switch (event) {
      case TM_ADD: SCALAR4(ADDF, nb, nc_v, r); break;
      case TM_SUB: SCALAR4(SUBF, nb, nc_v, r); break;
      case TM_MUL: SCALAR4(MULF, nb, nc_v, r); break;
      case TM_MOD: SCALAR4(l_vecop(fmod), nb, nc_v, r); break;
      case TM_POW: SCALAR4(l_vecop(pow), nb, nc_v, r); break;
      case TM_DIV:
        if (V_ISZERO(nc_v)) {
          ERR_DIVZERO(L);
        }

        SCALAR4(DIVF, nb, nc_v, r);
        break;
      case TM_IDIV:
        if (V_ISZERO(nc_v)) {
          ERR_DIVZERO(L);
        }

        SCALAR4(DIVF, nb, nc_v, r);
        OP4(l_vecop(floor), r, r);
        break;
      default:
        luaG_runerror(L, ERR_INVALID_OP, LABEL_VECTOR4, LABEL_NUMBER);
    }
    setvvalue(s2v(res), r, LUA_VVECTOR4);
  }
  else if (ttisnumber(p1) && nc_count == 3) {  /* <numeric, op, vec3> */
    lua_VecF nb_v = cast_vec(nvalue(p1));
    switch (event) {
      case TM_ADD: SCALAR3B(ADDF, nc, nb_v, r); break;
      case TM_SUB: SCALAR3B(SUBF, nc, nb_v, r); break;
      case TM_MUL: SCALAR3B(MULF, nc, nb_v, r); break;
      case TM_POW: SCALAR3B(l_vecop(pow), nc, nb_v, r); break;
      case TM_DIV:
        if (V_ISZERO(nc.x) || V_ISZERO(nc.y) || V_ISZERO(nc.z)) {
          ERR_DIVZERO(L);
        }

        SCALAR3B(DIVF, nc, nb_v, r);
        break;
      default:
        luaG_runerror(L, ERR_INVALID_OP, LABEL_NUMBER, LABEL_VECTOR3);
    }
    setvvalue(s2v(res), r, LUA_VVECTOR3);
  }
  else if (ttisnumber(p1) && nc_count == 2) {  /* <numeric, op, vec2> */
    lua_VecF nb_v = cast_vec(nvalue(p1));
    switch (event) {
      case TM_ADD: SCALAR2B(ADDF, nc, nb_v, r); break;
      case TM_SUB: SCALAR2B(SUBF, nc, nb_v, r); break;
      case TM_MUL: SCALAR2B(MULF, nc, nb_v, r); break;
      case TM_POW: SCALAR2B(l_vecop(pow), nc, nb_v, r); break;
      case TM_DIV:
        if (V_ISZERO(nc.x) || V_ISZERO(nc.y)) {
          ERR_DIVZERO(L);
        }

        SCALAR2B(DIVF, nc, nb_v, r);
        break;
      default:
        luaG_runerror(L, ERR_INVALID_OP, LABEL_NUMBER, LABEL_VECTOR2);
    }
    setvvalue(s2v(res), r, LUA_VVECTOR2);
  }
  else if (ttisnumber(p1) && nc_count == 4) {   /* <numeric, op, vec4> */
    lua_VecF nb_v = cast_vec(nvalue(p1));
    switch (event) {
      case TM_ADD: SCALAR4B(ADDF, nc, nb_v, r); break;
      case TM_SUB: SCALAR4B(SUBF, nc, nb_v, r); break;
      case TM_MUL: SCALAR4B(MULF, nc, nb_v, r); break;
      case TM_POW: SCALAR4B(l_vecop(pow), nc, nb_v, r); break;
      case TM_DIV:
        if (V_ISZERO(nc.x) || V_ISZERO(nc.y) || V_ISZERO(nc.z) || V_ISZERO(nc.w)) {
          ERR_DIVZERO(L);
        }

        SCALAR4B(DIVF, nc, nb_v, r);
        break;
      default:
        luaG_runerror(L, ERR_INVALID_OP, LABEL_NUMBER, LABEL_VECTOR4);
    }
    setvvalue(s2v(res), r, LUA_VVECTOR4);
  }
  else {
    result = 0;
  }
  return result;
}

/* }================================================================== */

/*
** {==================================================================
** Vector Math
** ===================================================================
*/

/* */
static LUA_INLINE void cross3 (lua_VecF x1, lua_VecF y1, lua_VecF z1,
                               lua_VecF x2, lua_VecF y2, lua_VecF z2,
                               lua_VecF *xr, lua_VecF *yr, lua_VecF *zr) {
  *xr = y1 * z2 - z1 * y2;
  *yr = z1 * x2 - x1 * z2;
  *zr = x1 * y2 - y1 * x2;
}

lua_Number luaVec_length2 (const lua_Float4 v) { return l_mathop(sqrt)(DOT2(cast_num, v, v)); }
lua_Number luaVec_length3 (const lua_Float4 v) { return l_mathop(sqrt)(DOT3(cast_num, v, v)); }
lua_Number luaVec_length4 (const lua_Float4 v) { return l_mathop(sqrt)(DOT4(cast_num, v, v)); }

int luaVec_angleaxis (const lua_Float4 v3, lua_VecF angle, lua_Float4 *q) {
  lua_VecF ha = angle * (V_HALF * V_PI / cast_vec(180.0));
  lua_VecF s = l_vecop(sin)(ha);

  q->w = l_vecop(cos)(ha);
  q->x = v3.x * s;
  q->y = v3.y * s;
  q->z = v3.z * s;
  return 1;
}

/* Based on Stan Melax's article in Game Programming Gems */
int luaVec_angle (const lua_Float4 a, const lua_Float4 b, lua_Float4 *q) {
  lua_VecF d;
  lua_Float4 v, v2;

  lua_VecF l1 = l_vecop(sqrt)(DOT3(cast_vec, a, a));
  lua_VecF l2 = l_vecop(sqrt)(DOT3(cast_vec, b, b));
  SCALAR3(DIVF, a, l1, v); /* Normalize the vectors */
  SCALAR3(DIVF, b, l2, v2);

  /* If dot == 1, vectors are the same */
  if ((d = DOT3(cast_vec, v, v2)) >= V_ONE) {
    q->w = V_ONE;
    q->x = q->y = q->z = V_ZERO;
  }
  else if (d < (LUA_VEC_NUMBER_EPS - V_ONE)) {
    lua_Float4 r;
    lua_VecF len;

    cross3(V_ONE, V_ZERO, V_ZERO, v.x, v.y, v.z, &r.x, &r.y, &r.z);
    if ((len = DOT3(cast_vec, r, r)) <= LUA_VEC_NUMBER_EPS) {
      cross3(V_ZERO, V_ONE, V_ZERO, v.x, v.y, v.z, &r.x, &r.y, &r.z);
      len = DOT3(cast_vec, r, r);
    }

    len = l_vecop(sqrt)(len);
    q->w = V_ZERO;
    q->x = r.x / len;
    q->y = r.y / len;
    q->z = r.z / len;
  }
  else {
    lua_Float4 r;
    lua_VecF len, s = l_vecop(sqrt)((V_ONE + d) * V_TWO);

    cross3(v.x, v.y, v.z, v2.x, v2.y, v2.z, &r.x, &r.y, &r.z);
    r.w = s * V_HALF;
    r.x /= s; r.y /= s; r.z /= s;

    len = l_vecop(sqrt)(DOT3(cast_vec, r, r));
    q->w = r.w / len;
    q->x = r.x / len;
    q->y = r.y / len;
    q->z = r.z / len;
  }
  return 1;
}

lua_Number luaVec_axisangle (const lua_Float4 v) {
  lua_Number angle = 2.0 * l_mathop(acos)(cast_num(v.w));
  return l_mathop(todeg)(angle);
}

int luaVec_axis (const lua_Float4 v, lua_Float4 *r) {
  if (l_vecop(fabs)(DOT4(cast_vec, v, v) - V_ONE) <= LUA_VEC_NUMBER_EPS) {
    lua_VecF rcpSin = V_ONE / l_vecop(sqrt)(V_ONE - v.w * v.w);
    r->w = V_ZERO;
    r->x = rcpSin * v.x;
    r->y = rcpSin * v.y;
    r->z = rcpSin * v.z;
  }
  else {
#if defined(GRIT_IDENTITY_ERROR)
    return 0;
#else
    r->x = r->y = r->z = r->w = V_ZERO;
#endif
  }
  return 1;
}

/* }================================================================== */

/*
** {==================================================================
** Vector Math
** ===================================================================
*/

int luaVec_dot (lua_State *L) {
  if (lua_gettop(L) != 2)
    return luaL_error(L, "Invalid params, try dot(v,v)");
  else if (lua_isnumber(L, 1))
    lua_pushnumber(L, luaL_checknumber(L, 1) * luaL_checknumber(L, 2));
  else {
    lua_Float4 v, v2;
    switch (lua_tovector(L, 1, V_PARSETABLE, &v)) {
      case LUA_VVECTOR1:
        lua_checkv1(L, 2, V_NOTABLE, &v2);
        lua_pushnumber(L, cast_num(v.x) * cast_num(v2.x));
        break;
      case LUA_VVECTOR2:
        lua_checkv2(L, 2, V_PARSETABLE, &v2);
        lua_pushnumber(L, DOT2(cast_num, v, v2));
        break;
      case LUA_VVECTOR3:
        lua_checkv3(L, 2, V_PARSETABLE, &v2);
        lua_pushnumber(L, DOT3(cast_num, v, v2));
        break;
      case LUA_VVECTOR4:
        lua_checkv4(L, 2, V_PARSETABLE, &v2);
        lua_pushnumber(L, DOT4(cast_num, v, v2));
        break;
      case LUA_VQUAT:
        lua_checkquat(L, 2, V_PARSETABLE, &v2);
        lua_pushnumber(L, DOT4(cast_num, v, v2));
        break;
      default:
        return luaL_typeerror(L, 1, LABEL_ALL);
    }
  }
  return 1;
}


int luaVec_cross (lua_State *L) {
  lua_Float4 v, v2, c;
  if (lua_gettop(L) != 2)
    return luaL_error(L, "Invalid params, try cross(v,v)");

  lua_checkv3(L, 1, V_PARSETABLE, &v);
  lua_checkv3(L, 2, V_PARSETABLE, &v2);

  c.x = v.y * v2.z - v.z * v2.y;
  c.y = v.z * v2.x - v.x * v2.z;
  c.z = v.x * v2.y - v.y * v2.x;
  c.w = V_ZERO;
  lua_pushvector(L, c, LUA_VVECTOR3);
  return 1;
}


/* don't invert w, as that would mean inv(Q_ID) would flip the polarity of w */
int luaVec_inv (lua_State *L) {
  lua_Float4 v;
  if (lua_gettop(L) != 1)
    return luaL_error(L, "Invalid params, try inv(q)");

  lua_checkquat(L, 1, V_PARSETABLE, &v);
  v.x *= -V_ONE;
  v.y *= -V_ONE;
  v.z *= -V_ONE;
  lua_pushvector(L, v, LUA_VQUAT);
  return 1;
}


int luaVec_norm (lua_State *L) {
  int variant;
  lua_VecF len;
  lua_Float4 v;
  if (lua_isnumber(L, 1)) {
    lua_pushnumber(L, l_mathop(1.0));
    return 1;
  }

  switch ((variant = lua_tovector(L, 1, V_PARSETABLE, &v))) {
    case LUA_VVECTOR1:  /* Technically should never be reached. */
      lua_pushnumber(L, l_mathop(1.0));
      return 1;
    case LUA_VVECTOR2:
      len = l_vecop(sqrt)(DOT2(cast_vec, v, v));
      if (V_ISZERO(len))
        return luaL_error(L, "Cannot normalize " LABEL_VECTOR2);
      SCALAR2(DIVF, v, len, v);
      break;
    case LUA_VVECTOR3:
      len = l_vecop(sqrt)(DOT3(cast_vec, v, v));
      if (V_ISZERO(len))
        return luaL_error(L, "Cannot normalize " LABEL_VECTOR3);
      SCALAR3(DIVF, v, len, v);
      break;
    case LUA_VVECTOR4:
      len = l_vecop(sqrt)(DOT4(cast_vec, v, v));
      if (V_ISZERO(len))
        return luaL_error(L, "Cannot normalize " LABEL_VECTOR4);
      SCALAR4(DIVF, v, len, v);
      break;
    case LUA_VQUAT:
      len = l_vecop(sqrt)(DOT4(cast_vec, v, v));
      if (V_ISZERO(len))
        return luaL_error(L, "Cannot normalize " LABEL_QUATERN);
      SCALAR4(DIVF, v, len, v);
      break;
    default:
      return luaL_typeerror(L, 1, "Invalid arguments, try norm(v) or norm(q).");
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_slerp (lua_State *L) {
  lua_VecF t, dot;
  lua_Float4 v, v2;
  if (lua_gettop(L) != 3)
    return luaL_error(L, "Invalid params, try slerp(q1, q2, a)");

  lua_checkquat(L, 1, V_PARSETABLE, &v);
  lua_checkquat(L, 2, V_PARSETABLE, &v2);
  t = cast_vec(lua_tonumber(L, 3));

  if ((dot = DOT4(cast_vec, v, v2)) < V_ZERO) {
    v2.x *= -V_ONE; /* flip one of them */
    v2.y *= -V_ONE;
    v2.z *= -V_ONE;
    v2.w *= -V_ONE;
    dot *= -V_ONE; /* update dot to match */
  }

  /*
  ** Due to rounding errors, even when vectors are normalised in Lua, dot can
  ** be > 1. We treat this case as if dot == 1 as it can only happen when the
  ** quats are very similar.
  */
  if (dot < V_ONE) {
    lua_VecF theta, s0, s1, d;
    theta = l_vecop(acos)(dot);
    s0 = l_vecop(sin)(theta * (V_ONE - t));
    s1 = l_vecop(sin)(theta * t);
    d = V_ONE / l_vecop(sin)(theta);

    v.w = (d * (v.w * s0 + v2.w * s1));
    v.x = (d * (v.x * s0 + v2.x * s1));
    v.y = (d * (v.y * s0 + v2.y * s1));
    v.z = (d * (v.z * s0 + v2.z * s1));
  }

  lua_pushvector(L, v, LUA_VQUAT);
  return 1;
}

int luaVec_abs (lua_State *L) { LUA_VEC_OP_UNARY(L, fabs); }
int luaVec_sin (lua_State *L) { LUA_VEC_OP_UNARY(L, sin); }
int luaVec_cos (lua_State *L) { LUA_VEC_OP_UNARY(L, cos); }
int luaVec_tan (lua_State *L) { LUA_VEC_OP_UNARY(L, tan); }
int luaVec_asin (lua_State *L) { LUA_VEC_OP_UNARY(L, asin); }
int luaVec_acos (lua_State *L) { LUA_VEC_OP_UNARY(L, acos); }
int luaVec_floor (lua_State *L) { LUA_VEC_OP_UNARY(L, floor); }
int luaVec_ceil (lua_State *L) { LUA_VEC_OP_UNARY(L, ceil); }
int luaVec_sqrt (lua_State *L) { LUA_VEC_OP_UNARY(L, sqrt); }
int luaVec_exp (lua_State *L) { LUA_VEC_OP_UNARY(L, exp); }
int luaVec_deg (lua_State *L) { LUA_VEC_OP_UNARY(L, todeg); }
int luaVec_rad (lua_State *L) { LUA_VEC_OP_UNARY(L, torad); }
int luaVec_fmod (lua_State *L) { LUA_VEC_OP_BINARY(L, fmod); }

int luaVec_atan (lua_State *L) {
  int variant;
  lua_Float4 v, v2;
  switch ((variant = lua_tovector(L, 1, V_PARSETABLE, &v))) {
    case LUA_VVECTOR1: {
      v2.x = V_ONE;
      if (lua_type(L, 2) != LUA_TNONE)
        v2.x = cast_vec(luaL_checknumber(L, 2));
      PW1(l_vecop(atan2), v, v2, v);
      break;
    }
    case LUA_VVECTOR2: {
      if (lua_type(L, 2) == LUA_TNONE)
        v2.x = v2.y = V_ONE;
      else if (lua_type(L, 2) == LUA_TVECTOR)
        lua_checkv2(L, 2, V_PARSETABLE, &v2);
      else { /* Assume it's a LUA_TNUMBER, throw an error otherwise. */
        v2.x = v2.y = cast_vec(luaL_checknumber(L, 2));
      }

      PW2(l_vecop(atan2), v, v2, v);
      break;
    }
    case LUA_VVECTOR3: {
      if (lua_type(L, 2) == LUA_TNONE)
        v2.x = v2.y = v2.z = V_ONE;
      else if (lua_type(L, 2) == LUA_TVECTOR)
        lua_checkv3(L, 2, V_PARSETABLE, &v2);
      else {
        v2.x = v2.y = v2.z = cast_vec(luaL_checknumber(L, 2));
      }

      PW3(l_vecop(atan2), v, v2, v);
      break;
    }
    case LUA_VVECTOR4: {
      if (lua_type(L, 2) == LUA_TNONE)
        v2.x = v2.y = v2.z = v2.w = V_ONE;
      else if (lua_type(L, 2) == LUA_TVECTOR)
        lua_checkv4(L, 2, V_PARSETABLE, &v2);
      else {
        v2.x = v2.y = v2.z = v2.w = cast_vec(luaL_checknumber(L, 2));
      }

      PW4(l_vecop(atan2), v, v2, v);
      break;
    }
    default:
      return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


/* from lmathlib.c */
static lua_VecF log_helper (lua_State *L, lua_VecF x) {
  lua_VecF res;
  if (lua_isnoneornil(L, 2))
    res = l_vecop(log)(x);
  else {
    lua_VecF base = cast_vec(luaL_checknumber(L, 2));
#if !defined(LUA_USE_C89)
    if (base == V_TWO)
      res = log2f(x); else
#endif
    if (base == l_vecop(10.0))
      res = l_vecop(log10)(x);
    else
      res = l_vecop(log)(x) / l_vecop(log)(base);
}
  return res;
}


int luaVec_log (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, V_PARSETABLE, &v))) {
    case LUA_VVECTOR4: v.w = log_helper(L, v.w); LUA_FALLTHROUGH;
    case LUA_VVECTOR3: v.z = log_helper(L, v.z); LUA_FALLTHROUGH;
    case LUA_VVECTOR2: v.y = log_helper(L, v.y); LUA_FALLTHROUGH;
    case LUA_VVECTOR1: v.x = log_helper(L, v.x); break;
    default:
      return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_min (lua_State *L) {
  int i, variant;
  int n = lua_gettop(L);  /* number of arguments */

  lua_Float4 v, v2;
  switch ((variant = lua_tovector(L, 1, V_PARSETABLE, &v))) {
    case LUA_VVECTOR1:
      for (i = 2; i <= n; i++) {
        lua_checkv1(L, i, V_NOTABLE, &v2);
        PW1(luai_numlt_c, v, v2, v);
      }
      break;
    case LUA_VVECTOR2:
      for (i = 2; i <= n; i++) {
        lua_checkv2(L, i, V_PARSETABLE, &v2);
        PW2(luai_numlt_c, v, v2, v);
      }
      break;
    case LUA_VVECTOR3:
      for (i = 2; i <= n; i++) {
        lua_checkv3(L, i, V_PARSETABLE, &v2);
        PW3(luai_numlt_c, v, v2, v);
      }
      break;
    case LUA_VVECTOR4:
      for (i = 2; i <= n; i++) {
        lua_checkv4(L, i, V_PARSETABLE, &v2);
        PW4(luai_numlt_c, v, v2, v);
      }
      break;
    default:
      return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_max (lua_State *L) {
  int i, variant;
  int n = lua_gettop(L);  /* number of arguments */

  lua_Float4 v, v2;
  switch ((variant = lua_tovector(L, 1, V_PARSETABLE, &v))) {
    case LUA_VVECTOR1:
      for (i = 2; i <= n; i++) {
        lua_checkv1(L, i, V_NOTABLE, &v2);
        PW1(luai_numgt_c, v, v2, v);
      }
      break;
    case LUA_VVECTOR2:
      for (i = 2; i <= n; i++) {
        lua_checkv2(L, i, V_PARSETABLE, &v2);
        PW2(luai_numgt_c, v, v2, v);
      }
      break;
    case LUA_VVECTOR3:
      for (i = 2; i <= n; i++) {
        lua_checkv3(L, i, V_PARSETABLE, &v2);
        PW3(luai_numgt_c, v, v2, v);
      }
      break;
    case LUA_VVECTOR4:
      for (i = 2; i <= n; i++) {
        lua_checkv4(L, i, V_PARSETABLE, &v2);
        PW4(luai_numgt_c, v, v2, v);
      }
      break;
    default:
      return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_clamp (lua_State *L) {
  int variant;
  lua_Float4 min, v, max;
  switch ((variant = lua_tovector(L, 1, V_PARSETABLE, &v))) {
    case LUA_VVECTOR1:
      lua_checkv1(L, 2, V_NOTABLE, &min);
      lua_checkv1(L, 3, V_NOTABLE, &max);
      PW1(luai_numgt_c, v, min, v);
      PW1(luai_numlt_c, v, max, v);
      break;
    case LUA_VVECTOR2:
      lua_checkv2(L, 2, V_PARSETABLE, &min);
      lua_checkv2(L, 3, V_PARSETABLE, &max);
      PW2(luai_numgt_c, v, min, v);
      PW2(luai_numlt_c, v, max, v);
      break;
    case LUA_VVECTOR3:
      lua_checkv3(L, 2, V_PARSETABLE, &min);
      lua_checkv3(L, 3, V_PARSETABLE, &max);
      PW3(luai_numgt_c, v, min, v);
      PW3(luai_numlt_c, v, max, v);
      break;
    case LUA_VVECTOR4:
      lua_checkv4(L, 2, V_PARSETABLE, &min);
      lua_checkv4(L, 3, V_PARSETABLE, &max);
      PW4(luai_numgt_c, v, min, v);
      PW4(luai_numlt_c, v, max, v);
      break;
    default:
      return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}

/* }================================================================== */

/*
** {==================================================================
** Deprecated functions (for compatibility only)
** ===================================================================
*/
#if defined(LUA_COMPAT_MATHLIB)

int luaV_sinh (lua_State *L) { LUA_VEC_OP_UNARY(L, sinh); }
int luaV_cosh (lua_State *L) { LUA_VEC_OP_UNARY(L, cosh); }
int luaV_tanh (lua_State *L) { LUA_VEC_OP_UNARY(L, tanh); }
int luaV_log10 (lua_State *L) { LUA_VEC_OP_UNARY(L, log10); }

int luaV_pow (lua_State *L) {
  if (lua_isquat(L, 1, V_PARSETABLE)) {
    lua_Float4 v;
    lua_VecF x, d, l;

    int variant = lua_tovector(L, 1, V_PARSETABLE, &v);
    if (lua_type(L, 2) != LUA_TNUMBER)
      return luaL_typeerror(L, 2, LABEL_NUMBER);

    x = cast_vec(luaL_checknumber(L, 2));
    d = DOT3(cast_vec, v, v);
    if ((l = l_vecop(sqrt)(d)) <= LUA_VEC_NUMBER_EPS) {
      v.w = V_ONE;
      v.x = v.y = v.z = V_ZERO;
    }
    else {
      lua_VecF angle = x * l_vecop(acos)(v.w); /* without the factor of 2 */
      lua_VecF sangle = l_vecop(sin)(angle);

      v.w = l_vecop(cos)(angle);
      v.x = sangle * v.x / l;
      v.y = sangle * v.y / l;
      v.z = sangle * v.z / l;
    }
    lua_pushvector(L, v, variant);
    return 1;
  }
  else {
    LUA_VEC_OP_BINARY(L, pow);
  }
}


#if defined(LUA_C99_MATHLIB)
int luaV_asinh (lua_State *L) { LUA_VEC_OP_UNARY(L, asinh); }
int luaV_acosh (lua_State *L) { LUA_VEC_OP_UNARY(L, acosh); }
int luaV_atanh (lua_State *L) { LUA_VEC_OP_UNARY(L, atanh); }
int luaV_cbrt (lua_State *L) { LUA_VEC_OP_UNARY(L, cbrt); }
int luaV_erf (lua_State *L) { LUA_VEC_OP_UNARY(L, erf); }
int luaV_erfc (lua_State *L) { LUA_VEC_OP_UNARY(L, erfc); }
int luaV_exp2 (lua_State *L) { LUA_VEC_OP_UNARY(L, exp2); }
int luaV_expm1 (lua_State *L) { LUA_VEC_OP_UNARY(L, expm1); }
int luaV_gamma (lua_State *L) { LUA_VEC_OP_UNARY(L, tgamma); }
int luaV_lgamma (lua_State *L) { LUA_VEC_OP_UNARY(L, lgamma); }
int luaV_log1p (lua_State *L) { LUA_VEC_OP_UNARY(L, log1p); }
int luaV_logb (lua_State *L) { LUA_VEC_OP_UNARY(L, logb); }
int luaV_nearbyint (lua_State *L) { LUA_VEC_OP_UNARY(L, nearbyint); }
int luaV_round (lua_State *L) { LUA_VEC_OP_UNARY(L, round); }
int luaV_trunc (lua_State *L) { LUA_VEC_OP_UNARY(L, trunc); }

int luaV_isfinite (lua_State *L) { LUA_VEC_CAND(L, isfinite); }
int luaV_isinf (lua_State *L) { LUA_VEC_COR(L, isinf); }
int luaV_isnan (lua_State *L) { LUA_VEC_COR(L, isnan); }
int luaV_isnormal (lua_State *L) { LUA_VEC_CAND(L, isnormal); }

int luaV_fdim (lua_State *L) { LUA_VEC_OP_BINARY(L, fdim); }
int luaV_hypot (lua_State *L) { LUA_VEC_OP_BINARY(L, hypot); }
int luaV_copysign (lua_State *L) { LUA_VEC_OP_BINARY(L, copysign); }
int luaV_nextafter (lua_State *L) { LUA_VEC_OP_BINARY(L, nextafter); }
int luaV_remainder (lua_State *L) { LUA_VEC_OP_BINARY(L, remainder); }

/* Second argument unsafely casted to an int... */
int luaV_scalbn (lua_State *L) {
  int variant;
  lua_Float4 v;

  int n = (int)luaL_checkinteger(L, 2);
  switch ((variant = lua_tovector(L, 1, V_PARSETABLE, &v))) {
    case LUA_VVECTOR4: v.w = l_vecop(scalbn)(v.w, n); LUA_FALLTHROUGH;
    case LUA_VVECTOR3: v.z = l_vecop(scalbn)(v.z, n); LUA_FALLTHROUGH;
    case LUA_VVECTOR2: v.y = l_vecop(scalbn)(v.y, n); LUA_FALLTHROUGH;
    case LUA_VVECTOR1: v.x = l_vecop(scalbn)(v.x, n); break;
    default:
      return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


#endif  /* LUA_C99_MATHLIB */
#endif

#ifdef _MSC_VER
__pragma(warning(pop))
#endif

/* }================================================================== */

