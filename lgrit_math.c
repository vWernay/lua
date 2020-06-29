/*
**
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

#define LABEL_NUMBER   "number"
#define LABEL_VECTOR2  "vector2"
#define LABEL_VECTOR3  "vector3"
#define LABEL_VECTOR4  "vector4"
#define LABEL_QUATERN  "quaternion"
#define LABEL_ALL      "number or vector type"

#define luai_numlt_c(a,b) luai_numlt(a, b) ? (a) : (b)
#define luai_numle_c(a,b) luai_numle(a, b) ? (a) : (b)
#define luai_numgt_c(a,b) luai_numgt(a, b) ? (a) : (b)
#define luai_numge_c(a,b) luai_numge(a, b) ? (a) : (b)

#define ADDF(x, y) ((x) + (y))
#define SUBF(x, y) ((x) - (y))
#define MULF(x, y) ((x) * (y))
#define DIVF(x, y) ((x) / (y))

#define OP1(f, lhs, r) { r.x = f(lhs.x); }
#define OP2(f, lhs, r) { r.x = f(lhs.x); r.y = f(lhs.y); }
#define OP3(f, lhs, r) { r.x = f(lhs.x); r.y = f(lhs.y); r.z = f(lhs.z); }
#define OP4(f, lhs, r) { r.x = f(lhs.x); r.y = f(lhs.y); r.z = f(lhs.z);  r.w = f(lhs.w); }
#define PW1(f, lhs, rhs, r) { r.x = f(lhs.x, rhs.x); }
#define PW2(f, lhs, rhs, r) { r.x = f(lhs.x, rhs.x); r.y = f(lhs.y, rhs.y); }
#define PW3(f, lhs, rhs, r) { r.x = f(lhs.x, rhs.x); r.y = f(lhs.y, rhs.y); r.z = f(lhs.z, rhs.z); }
#define PW4(f, lhs, rhs, r) { r.x = f(lhs.x, rhs.x); r.y = f(lhs.y, rhs.y); r.z = f(lhs.z, rhs.z);  r.w = f(lhs.w, rhs.w); }
#define SCALAR1(f, lhs, rhs, r) { r.x = f(lhs.x, rhs); }
#define SCALAR2(f, lhs, rhs, r) { r.x = f(lhs.x, rhs); r.y = f(lhs.y, rhs); }
#define SCALAR3(f, lhs, rhs, r) { r.x = f(lhs.x, rhs); r.y = f(lhs.y, rhs); r.z = f(lhs.z, rhs); }
#define SCALAR4(f, lhs, rhs, r) { r.x = f(lhs.x, rhs); r.y = f(lhs.y, rhs); r.z = f(lhs.z, rhs);  r.w = f(lhs.w, rhs); }
#define SCALAR21(f, lhs, rhs, r) { r.x = f(rhs, lhs.x); }
#define SCALAR2B(f, lhs, rhs, r) { r.x = f(rhs, lhs.x); r.y = f(rhs, lhs.y); }
#define SCALAR3B(f, lhs, rhs, r) { r.x = f(rhs, lhs.x); r.y = f(rhs, lhs.y); r.z = f(rhs, lhs.z); }
#define SCALAR4B(f, lhs, rhs, r) { r.x = f(rhs, lhs.x); r.y = f(rhs, lhs.y); r.z = f(rhs, lhs.z);  r.w = f(rhs, lhs.w); }

#define DOT2(c, lhs, rhs) ((c(lhs.x) * c(rhs.x)) + (c(lhs.y) * c(rhs.y)))
#define DOT3(c, lhs, rhs) ((c(lhs.x) * c(rhs.x)) + (c(lhs.y) * c(rhs.y)) + (c(lhs.z) * c(rhs.z)))
#define DOT4(c, lhs, rhs) ((c(lhs.x) * c(rhs.x)) + (c(lhs.y) * c(rhs.y)) + (c(lhs.z) * c(rhs.z)) + (c(lhs.w) * c(rhs.w)))

#define ISNORM2(lhs) (l_vecop(fabs)(DOT2(cast_vec, v, v) - V_ONE) <= LUA_VEC_NUMBER_EPS)
#define ISNORM3(lhs) (l_vecop(fabs)(DOT3(cast_vec, v, v) - V_ONE) <= LUA_VEC_NUMBER_EPS)
#define ISNORM4(lhs) (l_vecop(fabs)(DOT4(cast_vec, v, v) - V_ONE) <= LUA_VEC_NUMBER_EPS)

static LUA_INLINE float todegf(float x) { return x * (180.f / 3.141592653589793238462643383279502884f); }
static LUA_INLINE double todeg(double x) { return x * (180.0 / 3.141592653589793238462643383279502884); }
static LUA_INLINE long double todegl(long double x) { return x * (180.0 / 3.141592653589793238462643383279502884); }

static LUA_INLINE float toradf(float x) { return x * (3.141592653589793238462643383279502884f / 180.f); }
static LUA_INLINE double torad(double x) { return x * (3.141592653589793238462643383279502884 / 180.0); }
static LUA_INLINE long double toradl(long double x) { return x * (3.141592653589793238462643383279502884 / 180.0); }

/*
** {==================================================================
** Tag Methods
** ===================================================================
*/

/* Handle l_noret & unreachable code warnings */
#define ERR_DIVZERO(L) { luaG_runerror(L, "division by zero"); }

int luaVec_trybinTM (lua_State *L, const TValue *p1, const TValue *p2, StkId res, TMS event) {
  int result = 1;
  if (ttisvector3(p1) && ttisvector3(p2)) {
    lua_Float4 nb = vvalue(p1), nc = vvalue(p2), r = V_ZEROVEC;
    switch (event) {
      case TM_ADD:
        PW3(ADDF, nb, nc, r);
        break;
      case TM_SUB:
        PW3(SUBF, nb, nc, r);
        break;
      case TM_MUL:
        PW3(MULF, nb, nc, r);
        break;
      case TM_DIV:
        if (nc.x == V_ZERO || nc.y == V_ZERO || nc.z == V_ZERO) { ERR_DIVZERO(L); }
        PW3(DIVF, nb, nc, r);
        break;
      case TM_IDIV:
        if (nc.x == V_ZERO || nc.y == V_ZERO || nc.z == V_ZERO) { ERR_DIVZERO(L); }
        PW3(DIVF, nb, nc, r);
        OP3(l_vecop(floor), r, r);
        break;
      case TM_MOD:
        PW3(l_vecop(fmod), nb, nc, r);
        break;
      case TM_POW:
        PW3(l_vecop(pow), nb, nc, r);
        break;
      case TM_UNM:
        r.x = -nb.x; r.y = -nb.y; r.z = -nb.z;
        break;
      default:
        lua_assert(0);
        return 0;
    }
    setvvalue(s2v(res), r, LUA_VVECTOR3);
  }
  else if (ttisvector2(p1) && ttisvector2(p2)) {
    lua_Float4 nb = vvalue(p1), nc = vvalue(p2), r = V_ZEROVEC;
    switch (event) {
      case TM_ADD:
        PW2(ADDF, nb, nc, r);
        break;
      case TM_SUB:
        PW2(SUBF, nb, nc, r);
        break;
      case TM_MUL:
        PW2(MULF, nb, nc, r);
        break;
      case TM_DIV:
        if (nc.x == V_ZERO || nc.y == V_ZERO) { ERR_DIVZERO(L); }
        PW2(DIVF, nb, nc, r);
        break;
      case TM_IDIV:
        if (nc.x == V_ZERO || nc.y == V_ZERO) { ERR_DIVZERO(L); }
        PW2(DIVF, nb, nc, r);
        OP2(l_vecop(floor), r, r);
        break;
      case TM_MOD:
        PW2(l_vecop(fmod), nb, nc, r);
        break;
      case TM_POW:
        PW2(l_vecop(pow), nb, nc, r);
        break;
      case TM_UNM:
        r.x = -nb.x; r.y = -nb.y;
        break;
      default:
        lua_assert(0);
        return 0;
    }
    setvvalue(s2v(res), r, LUA_VVECTOR2);
  }
  else if (ttisvector4(p1) && ttisvector4(p2)) {
    lua_Float4 nb = vvalue(p1), nc = vvalue(p2), r = V_ZEROVEC;
    switch (event) {
      case TM_ADD:
        PW4(ADDF, nb, nc, r);
        break;
      case TM_SUB:
        PW4(SUBF, nb, nc, r);
        break;
      case TM_MUL:
        PW4(MULF, nb, nc, r);
        break;
      case TM_DIV:
        if (nc.x == V_ZERO || nc.y == V_ZERO || nc.z == V_ZERO || nc.w == V_ZERO) {
          ERR_DIVZERO(L);
        }
        PW4(DIVF, nb, nc, r);
        break;
      case TM_IDIV:
        if (nc.x == V_ZERO || nc.y == V_ZERO || nc.z == V_ZERO) { ERR_DIVZERO(L); }
        PW4(DIVF, nb, nc, r);
        OP4(l_vecop(floor), r, r);
        break;
      case TM_MOD:
        PW4(l_vecop(fmod), nb, nc, r);
        break;
      case TM_POW:
        PW4(l_vecop(pow), nb, nc, r);
        break;
      case TM_UNM:
        r.x = -nb.x; r.y = -nb.y; r.z = -nb.z; r.w = -nb.w;
        break;
      default:
        lua_assert(0);
        return 0;
    }
    setvvalue(s2v(res), r, LUA_VVECTOR4);
  }
  else if (ttisvector3(p1) && ttisnumber(p2)) {
    lua_Float4 nb = vvalue(p1), r = V_ZEROVEC;
    lua_VecF nc = cast_vec(nvalue(p2));
    switch (event) {
      case TM_ADD:
        SCALAR3(ADDF, nb, nc, r);
        break;
      case TM_SUB:
        SCALAR3(SUBF, nb, nc, r);
        break;
      case TM_MUL:
        SCALAR3(MULF, nb, nc, r);
        break;
      case TM_DIV:
        if (nc == V_ZERO) { ERR_DIVZERO(L); }
        SCALAR3(DIVF, nb, nc, r);
        break;
      case TM_IDIV:
        if (nc == V_ZERO) { ERR_DIVZERO(L); }
        SCALAR3(DIVF, nb, nc, r);
        OP3(l_vecop(floor), r, r);
        break;
      case TM_MOD:
        SCALAR3(l_vecop(fmod), nb, nc, r);
        break;
      case TM_POW:
        SCALAR3(l_vecop(pow), nb, nc, r);
        break;
      default:
        luaG_runerror(L, "Cannot use that op with vector3 and number");
    }
    setvvalue(s2v(res), r, LUA_VVECTOR3);
  }
  else if (ttisvector2(p1) && ttisnumber(p2)) {
    lua_Float4 nb = vvalue(p1), r = V_ZEROVEC;
    lua_VecF nc = cast_vec(nvalue(p2));
    switch (event) {
      case TM_ADD:
        SCALAR2(ADDF, nb, nc, r);
        break;
      case TM_SUB:
        SCALAR2(SUBF, nb, nc, r);
        break;
      case TM_MUL:
        SCALAR2(MULF, nb, nc, r);
        break;
      case TM_DIV:
        if (nc == V_ZERO) { ERR_DIVZERO(L); }
        SCALAR2(DIVF, nb, nc, r);
        break;
      case TM_IDIV:
        if (nc == V_ZERO) { ERR_DIVZERO(L); }
        SCALAR2(DIVF, nb, nc, r);
        OP2(l_vecop(floor), r, r);
        break;
      case TM_MOD:
        SCALAR2(l_vecop(fmod), nb, nc, r);
        break;
      case TM_POW:
        SCALAR2(l_vecop(pow), nb, nc, r);
        break;
      default:
        luaG_runerror(L, "Cannot use that op with vector2 and number");
    }
    setvvalue(s2v(res), r, LUA_VVECTOR2);
  }
  else if (ttisvector4(p1) && ttisnumber(p2)) {
    lua_Float4 nb = vvalue(p1), r = V_ZEROVEC;
    lua_VecF nc = cast_vec(nvalue(p2));
    switch (event) {
      case TM_ADD:
        SCALAR4(ADDF, nb, nc, r);
        break;
      case TM_SUB:
        SCALAR4(SUBF, nb, nc, r);
        break;
      case TM_MUL:
        SCALAR4(MULF, nb, nc, r);
        break;
      case TM_DIV:
        if (nc == V_ZERO) { ERR_DIVZERO(L); }
        SCALAR4(DIVF, nb, nc, r);
        break;
      case TM_IDIV:
        if (nc == V_ZERO) { ERR_DIVZERO(L); }
        SCALAR4(DIVF, nb, nc, r);
        OP4(l_vecop(floor), r, r);
        break;
      case TM_MOD:
        SCALAR4(l_vecop(fmod), nb, nc, r);
        break;
      case TM_POW:
        SCALAR4(l_vecop(pow), nb, nc, r);
        break;
      default:
        luaG_runerror(L, "Cannot use that op with vector4 and number");
    }
    setvvalue(s2v(res), r, LUA_VVECTOR4);
  }
  else if (ttisnumber(p1) && ttisvector3(p2)) {
    lua_Float4 nb = vvalue(p2), r = V_ZEROVEC;
    lua_VecF nc = cast_vec(nvalue(p1));
    switch (event) {
      case TM_ADD:
        SCALAR3B(ADDF, nb, nc, r);
        break;
      case TM_SUB:
        SCALAR3B(SUBF, nb, nc, r);
        break;
      case TM_MUL:
        SCALAR3B(MULF, nb, nc, r);
        break;
      case TM_DIV:
        if (nb.x == V_ZERO || nb.y == V_ZERO || nb.z == V_ZERO) { ERR_DIVZERO(L); }
        SCALAR3B(DIVF, nb, nc, r);
        break;
      case TM_POW:
        SCALAR3B(l_vecop(pow), nb, nc, r);
        break;
      default:
        luaG_runerror(L, "Cannot use that op with number and vector3");
    }
    setvvalue(s2v(res), r, LUA_VVECTOR3);
  }
  else if (ttisnumber(p1) && ttisvector2(p2)) {
    lua_Float4 nb = vvalue(p2), r = V_ZEROVEC;
    lua_VecF nc = cast_vec(nvalue(p1));
    switch (event) {
      case TM_ADD:
        SCALAR2B(ADDF, nb, nc, r);
        break;
      case TM_SUB:
        SCALAR2B(SUBF, nb, nc, r);
        break;
      case TM_MUL:
        SCALAR2B(MULF, nb, nc, r);
        break;
      case TM_DIV:
        if (nb.x == V_ZERO || nb.y == V_ZERO) { ERR_DIVZERO(L); }
        SCALAR2B(DIVF, nb, nc, r);
        break;
      case TM_POW:
        SCALAR2B(l_vecop(pow), nb, nc, r);
        break;
      default:
        luaG_runerror(L, "Cannot use that op with number and vector2");
    }
    setvvalue(s2v(res), r, LUA_VVECTOR2);
  }
  else if (ttisnumber(p1) && ttisvector4(p2)) {
    lua_Float4 nb = vvalue(p2), r = V_ZEROVEC;
    lua_VecF nc = cast_vec(nvalue(p1));
    switch (event) {
      case TM_ADD:
        SCALAR4B(ADDF, nb, nc, r);
        break;
      case TM_SUB:
        SCALAR4B(SUBF, nb, nc, r);
        break;
      case TM_MUL:
        SCALAR4B(MULF, nb, nc, r);
        break;
      case TM_DIV:
        if (nb.x == V_ZERO || nb.y == V_ZERO || nb.z == V_ZERO || nb.w == V_ZERO) {
          ERR_DIVZERO(L);
        }
        SCALAR4B(DIVF, nb, nc, r);
        break;
      case TM_POW:
        SCALAR4B(l_vecop(pow), nb, nc, r);
        break;
      default:
        luaG_runerror(L, "Cannot use that op with number and vector4");
    }
    setvvalue(s2v(res), r, LUA_VVECTOR4);
  }
  else if (ttisquat(p1) && ttisquat(p2)) {
    lua_Float4 nb = vvalue(p1), nc = vvalue(p2), r = V_ZEROVEC;
    switch (event) {
      case TM_MUL:
        r.w = nb.w * nc.w - nb.x * nc.x - nb.y * nc.y - nb.z * nc.z;
        r.x = nb.w * nc.x + nb.x * nc.w + nb.y * nc.z - nb.z * nc.y;
        r.y = nb.w * nc.y + nb.y * nc.w + nb.z * nc.x - nb.x * nc.z;
        r.z = nb.w * nc.z + nb.z * nc.w + nb.x * nc.y - nb.y * nc.x;
        break;
      default:
        luaG_runerror(L, "Cannot use that op with quat and quat");
    }
    setvvalue(s2v(res), r, LUA_VQUAT);
  }
  else if (ttisquat(p1) && ttisvector3(p2)) {
    lua_Float4 nb = vvalue(p1), nc = vvalue(p2), r = V_ZEROVEC;
    switch (event) {
      case TM_MUL: {
        const lua_VecF a = nb.w, b = nb.x, c = nb.y, d = nb.z;
        lua_VecF mat[3][3] = { /* row major */
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
        luaG_runerror(L, "Cannot use that op with quat and vector3");
    }
    setvvalue(s2v(res), r, LUA_VVECTOR3);
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
#define PI   (l_mathop(3.141592653589793238462643383279502884))

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
  lua_Number angle = 2.0 * l_mathop(acos)(v.w);
  return l_mathop(todeg)(angle);
}

int luaVec_axis (const lua_Float4 v, lua_Float4 *r) {
  if (ISNORM4(v)) {
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
    switch (lua_tovector(L, 1, &v)) {
      case LUA_VNUMFLT:
        lua_checkv1(L, 2, &v2);
        lua_pushnumber(L, cast_num(v.x) * cast_num(v2.x));
        break;
      case LUA_VVECTOR2:
        lua_checkv2(L, 2, &v2);
        lua_pushnumber(L, DOT2(cast_num, v, v2));
        break;
      case LUA_VVECTOR3:
        lua_checkv3(L, 2, &v2);
        lua_pushnumber(L, DOT3(cast_num, v, v2));
        break;
      case LUA_VVECTOR4:
        lua_checkv4(L, 2, &v2);
        lua_pushnumber(L, DOT4(cast_num, v, v2));
        break;
      case LUA_VQUAT:
        lua_checkquat(L, 2, &v2);
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

  lua_checkv3(L, 1, &v);
  lua_checkv3(L, 2, &v2);

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

  lua_checkquat(L, 1, &v);
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

  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT:  /* Technically should never be reached. */
      lua_pushnumber(L, l_mathop(1.0));
      return 1;
    case LUA_VVECTOR2:
      if ((len = l_vecop(sqrt)(DOT2(cast_vec, v, v))) == V_ZERO)
        return luaL_typeerror(L, 1, "Cannot normalize vector2");
      SCALAR2(DIVF, v, len, v);
      break;
    case LUA_VVECTOR3:
      if ((len = l_vecop(sqrt)(DOT3(cast_vec, v, v))) == V_ZERO)
        return luaL_typeerror(L, 1, "Cannot normalize vector3");
      SCALAR3(DIVF, v, len, v);
      break;
    case LUA_VVECTOR4:
      if ((len = l_vecop(sqrt)(DOT4(cast_vec, v, v))) == V_ZERO)
        return luaL_typeerror(L, 1, "Cannot normalize vector4");
      SCALAR4(DIVF, v, len, v);
      break;
    case LUA_VQUAT:
      if ((len = l_vecop(sqrt)(DOT4(cast_vec, v, v))) == V_ZERO)
        return luaL_typeerror(L, 1, "Cannot normalize quat");
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

  lua_checkquat(L, 1, &v);
  lua_checkquat(L, 2, &v2);
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


int luaVec_abs (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(fabs)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(fabs), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(fabs), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(fabs), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_sin (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(sin)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(sin), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(sin), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(sin), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_cos (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(cos)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(cos), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(cos), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(cos), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_tan (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(tan)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(tan), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(tan), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(tan), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_asin (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(asin)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(asin), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(asin), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(asin), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_acos (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(acos)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(acos), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(acos), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(acos), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_atan (lua_State *L) {
  int variant;
  lua_Float4 v, v2;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: {
      v2.x = V_ONE;
      if (lua_type(L, 2) != LUA_TNONE)
        v2.x = cast_vec(luaL_checknumber(L, 2));
      lua_pushnumber(L, l_vecop(atan2)(v.x, v2.x));
      return 1;
    }
    case LUA_VVECTOR2: {
      if (lua_type(L, 2) == LUA_TNONE)
        v2.x = v2.y = V_ONE;
      else if (lua_type(L, 2) == LUA_TVECTOR)
        lua_checkv2(L, 2, &v2);
      else /* Assume it's a LUA_TNUMBER, throw an error otherwise. */
        v2.x = v2.y = cast_vec(luaL_checknumber(L, 2));

      PW2(l_vecop(atan2), v, v2, v);
      break;
    }
    case LUA_VVECTOR3: {
      if (lua_type(L, 2) == LUA_TNONE)
        v2.x = v2.y = v2.z = V_ONE;
      else if (lua_type(L, 2) == LUA_TVECTOR)
        lua_checkv3(L, 2, &v2);
      else
        v2.x = v2.y = v2.z = cast_vec(luaL_checknumber(L, 2));

      PW3(l_vecop(atan2), v, v2, v);
      break;
    }
    case LUA_VVECTOR4: {
      if (lua_type(L, 2) == LUA_TNONE)
        v2.x = v2.y = v2.z = v2.w = V_ONE;
      else if (lua_type(L, 2) == LUA_TVECTOR)
        lua_checkv4(L, 2, &v2);
      else
        v2.x = v2.y = v2.z = v2.w = cast_vec(luaL_checknumber(L, 2));

      PW4(l_vecop(atan2), v, v2, v);
      break;
    }
    default:
      return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_floor (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(floor)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(floor), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(floor), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(floor), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_ceil (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(ceil)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(ceil), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(ceil), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(ceil), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_fmod (lua_State *L) {
  int variant;
  lua_Float4 v, v2;
  lua_VecF x;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT:
      x = cast_vec(luaL_checknumber(L, 2));
      lua_pushnumber(L, l_mathop(fmod)(v.x, x));
      return 1;
    case LUA_VVECTOR2:
      if (lua_type(L, 2) == LUA_TNUMBER) {
        x = cast_vec(luaL_checknumber(L, 2));
        SCALAR2(l_vecop(fmod), v, x, v);
      }
      else if (lua_type(L, 2) == LUA_TVECTOR) {
        lua_checkv2(L, 2, &v2);
        PW2(l_vecop(fmod), v, v2, v);
      }
      else
        return luaL_typeerror(L, 2, LABEL_NUMBER " or " LABEL_VECTOR2);
      break;
    case LUA_VVECTOR3:
      if (lua_type(L, 2) == LUA_TNUMBER) {
        x = cast_vec(luaL_checknumber(L, 2));
        SCALAR3(l_vecop(fmod), v, x, v);
      }
      else if (lua_type(L, 2) == LUA_TVECTOR) {
        lua_checkv3(L, 2, &v2);
        PW3(l_vecop(fmod), v, v2, v);
      }
      else
        return luaL_typeerror(L, 2, LABEL_NUMBER " or " LABEL_VECTOR3);
      break;
    case LUA_VVECTOR4:
      if (lua_type(L, 2) == LUA_TNUMBER) {
        x = cast_vec(luaL_checknumber(L, 2));
        SCALAR3(l_vecop(fmod), v, x, v);
      }
      else if (lua_type(L, 2) == LUA_TVECTOR) {
        lua_checkv4(L, 2, &v2);
        PW4(l_vecop(fmod), v, v2, v);
      }
      else
        return luaL_typeerror(L, 2, LABEL_NUMBER " or " LABEL_VECTOR2);
      break;
    default:
      return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_sqrt (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(sqrt)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(sqrt), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(sqrt), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(sqrt), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
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
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT:
      lua_pushnumber(L, cast_num(log_helper(L, v.x)));
      return 1;
    case LUA_VVECTOR4: v.w = log_helper(L, v.w); /* FALLTHROUGH */
    case LUA_VVECTOR3: v.z = log_helper(L, v.z); /* FALLTHROUGH */
    case LUA_VVECTOR2:
      v.y = log_helper(L, v.y);
      v.x = log_helper(L, v.x);
      break;
    default:
      return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_exp (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(exp)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(exp), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(exp), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(exp), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_deg (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT:
      lua_pushnumber(L, cast_num(v.x) * (l_mathop(3.141592653589793238462643383279502884) / l_mathop(180.0)));
      return 1;
    case LUA_VVECTOR2: OP2(l_vecop(todeg), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(todeg), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(todeg), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_rad (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT:
      lua_pushnumber(L, cast_num(v.x) * (l_mathop(180.0) / l_mathop(3.141592653589793238462643383279502884)));
      return 1;
    case LUA_VVECTOR2: OP2(l_vecop(torad), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(torad), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(torad), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaVec_min (lua_State *L) {
  int i, variant;
  int n = lua_gettop(L);  /* number of arguments */

  lua_Float4 v, v2;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT:
      for (i = 2; i <= n; i++) {
        lua_checkv1(L, i, &v2);
        PW1(luai_numlt_c, v, v2, v);
      }
      lua_pushnumber(L, cast_num(v.x));
      return 1;
    case LUA_VVECTOR2:
      for (i = 2; i <= n; i++) {
        lua_checkv2(L, i, &v2);
        PW2(luai_numlt_c, v, v2, v);
      }
      break;
    case LUA_VVECTOR3:
      for (i = 2; i <= n; i++) {
        lua_checkv3(L, i, &v2);
        PW3(luai_numlt_c, v, v2, v);
      }
      break;
    case LUA_VVECTOR4:
      for (i = 2; i <= n; i++) {
        lua_checkv4(L, i, &v2);
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
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT:
      for (i = 2; i <= n; i++) {
        lua_checkv1(L, i, &v2);
        PW1(luai_numgt_c, v, v2, v);
      }
      lua_pushnumber(L, cast_num(v.x));
      return 1;
    case LUA_VVECTOR2:
      for (i = 2; i <= n; i++) {
        lua_checkv2(L, i, &v2);
        PW2(luai_numgt_c, v, v2, v);
      }
      break;
    case LUA_VVECTOR3:
      for (i = 2; i <= n; i++) {
        lua_checkv3(L, i, &v2);
        PW3(luai_numgt_c, v, v2, v);
      }
      break;
    case LUA_VVECTOR4:
      for (i = 2; i <= n; i++) {
        lua_checkv4(L, i, &v2);
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
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT:
      lua_checkv1(L, 2, &min); lua_checkv1(L, 3, &max);
      PW2(luai_numgt_c, v, min, v);
      PW2(luai_numlt_c, v, max, v);
      lua_pushnumber(L, cast_num(v.x));
      return 1;
    case LUA_VVECTOR2:
      lua_checkv2(L, 2, &min); lua_checkv2(L, 3, &max);
      PW2(luai_numgt_c, v, min, v);
      PW2(luai_numlt_c, v, max, v);
      break;
    case LUA_VVECTOR3:
      lua_checkv3(L, 2, &min); lua_checkv3(L, 3, &max);
      PW3(luai_numgt_c, v, min, v);
      PW3(luai_numlt_c, v, max, v);
      break;
    case LUA_VVECTOR4:
      lua_checkv4(L, 2, &min); lua_checkv4(L, 3, &max);
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

int luaV_sinh (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(sinh)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(sinh), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(sinh), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(sinh), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaV_cosh (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(cosh)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(cosh), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(cosh), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(cosh), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaV_tanh (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(tanh)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(tanh), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(tanh), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(tanh), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaV_pow (lua_State *L) {
  int variant;
  lua_Float4 v, v2;
  lua_VecF x;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT:
      x = cast_vec(luaL_checknumber(L, 2));
      lua_pushnumber(L, l_mathop(pow)(v.x, x));
      return 1;
    case LUA_VVECTOR2:
      if (lua_type(L, 2) == LUA_TNUMBER) {
        x = cast_vec(luaL_checknumber(L, 2));
        SCALAR2(l_vecop(pow), v, x, v);
      }
      else if (lua_type(L, 2) == LUA_TVECTOR) {
        lua_checkv2(L, 2, &v2);
        PW2(l_vecop(pow), v, v2, v);
      }
      else
        return luaL_typeerror(L, 2, LABEL_VECTOR2);
      break;
    case LUA_VVECTOR3:
      if (lua_type(L, 2) == LUA_TNUMBER) {
        x = cast_vec(luaL_checknumber(L, 2));
        SCALAR3(l_vecop(pow), v, x, v);
      }
      else if (lua_type(L, 2) == LUA_TVECTOR) {
        lua_checkv3(L, 2, &v2);
        PW3(l_vecop(pow), v, v2, v);
      }
      else
        return luaL_typeerror(L, 2, LABEL_VECTOR3);
      break;
    case LUA_VVECTOR4:
      if (lua_type(L, 2) == LUA_TNUMBER) {
        x = cast_vec(luaL_checknumber(L, 2));
        SCALAR3(l_vecop(pow), v, x, v);
      }
      else if (lua_type(L, 2) == LUA_TVECTOR) {
        lua_checkv4(L, 2, &v2);
        PW4(l_vecop(pow), v, v2, v);
      }
      else
        return luaL_typeerror(L, 2, LABEL_VECTOR4);
      break;
    case LUA_VQUAT: {
      lua_VecF l;
      if (lua_type(L, 2) != LUA_TNUMBER)
        return luaL_typeerror(L, 2, LABEL_NUMBER);

      x = cast_vec(luaL_checknumber(L, 2));
      if ((l = l_vecop(sqrt)(DOT3(cast_vec, v, v))) <= LUA_VEC_NUMBER_EPS) {
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
      break;
    }
    default:
      return luaL_error(L, "use math.pow(number, number) or math.pow(quat, number) or math.pow(vec, vec) or math.pow(vec, number)");
  }
  lua_pushvector(L, v, variant);
  return 1;
}


int luaV_log10 (lua_State *L) {
  int variant;
  lua_Float4 v;
  switch ((variant = lua_tovector(L, 1, &v))) {
    case LUA_VNUMFLT: lua_pushnumber(L, l_mathop(log10)(cast_num(v.x))); return 1;
    case LUA_VVECTOR2: OP2(l_vecop(log10), v, v); break;
    case LUA_VVECTOR3: OP3(l_vecop(log10), v, v); break;
    case LUA_VVECTOR4: OP4(l_vecop(log10), v, v); break;
    default: return luaL_typeerror(L, 1, LABEL_ALL);
  }
  lua_pushvector(L, v, variant);
  return 1;
}


#endif


/* }================================================================== */

