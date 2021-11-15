/*
** $Id: lglm_string.hpp $
** Stack-based glm/gtx/string_cast.inl implementation
** See Copyright Notice in lua.h
*/

#ifndef lglm_string_h
#define lglm_string_h

#include <cstdio>
#include <cctype>
#include <glm/glm.hpp>

/*
** @COMPAT GCC explicitly forbids using forceinline on variadic functions.
**  Compiling with glm/gtx/string_cast with GLM_FORCE_INLINE will lead to errors
*/
#if (GLM_COMPILER & GLM_COMPILER_GCC)
  #define GLM_STRING_FUNC_QUALIFIER
#else
  #define GLM_STRING_FUNC_QUALIFIER GLM_FUNC_QUALIFIER
#endif

/*
** {==================================================================
** glm::to_string implemented on the stack
** ===================================================================
*/

/* Buffer size of format header */
#define GLM_FORMAT_BUFFER 256

/* Simplify template casting */
#define GLM_STRING_CAST(X) static_cast<typename lglmcast<T>::value_type>((X))

/* Common lglm_compute_to_string header */
#define GLM_STRING_HEADER                                                              \
  char const *PrefixStr = lglmprefix<T>::value();                                      \
  char const *LiteralStr = lglmliteral<T, std::numeric_limits<T>::is_iec559>::value(); \
  char format_text[GLM_FORMAT_BUFFER];

/* Forward declare functor for pushing Lua strings without intermediate std::string */
namespace glm {
namespace detail {
  static GLM_STRING_FUNC_QUALIFIER int _vsnprintf(char *buff, size_t buff_len, const char *msg, ...) {
    int length = 0;

    va_list list;
    va_start(list, msg);
#if (GLM_COMPILER & GLM_COMPILER_VC)
    length = vsprintf_s(buff, buff_len, msg, list);
#else
    length = vsnprintf(buff, buff_len, msg, list);
#endif
    va_end(list);

    assert(length > 0);
    return length;
  }

  /// <summary>
  /// string_cast.inl: cast without the dependency
  /// </summary>
  template<typename T> struct lglmcast { typedef T value_type; };
  template<> struct lglmcast<float> { typedef double value_type; };

  /// <summary>
  /// string_cast.inl: literal without the dependency
  /// </summary>
  template<typename T, bool isFloat = false>
  struct lglmliteral { GLM_FUNC_QUALIFIER static char const *value() { return "%d"; } };

  /// <summary>
  /// @TODO Use the predefined literal floating-point format defined in
  /// luaconf.h if possible
  /// </summary>
  template<typename T>
  struct lglmliteral<T, true> { GLM_FUNC_QUALIFIER static char const *value() { return "%f"; } };
#if GLM_MODEL == GLM_MODEL_32 && GLM_COMPILER && GLM_COMPILER_VC
  template<> struct lglmliteral<int64_t, false> { GLM_FUNC_QUALIFIER static char const *value() { return "%lld"; } };
  template<> struct lglmliteral<uint64_t, false> { GLM_FUNC_QUALIFIER static char const *value() { return "%lld"; } };
#endif  // GLM_MODEL == GLM_MODEL_32 && GLM_COMPILER && GLM_COMPILER_VC

  /// <summary>
  /// string_cast.inl: prefix without the dependency
  /// </summary>
  template<typename T> struct lglmprefix{};
  template<> struct lglmprefix<float> { GLM_FUNC_QUALIFIER static char const * value() { return ""; } };
  template<> struct lglmprefix<double> { GLM_FUNC_QUALIFIER static char const * value() { return "d"; } };
  template<> struct lglmprefix<bool> { GLM_FUNC_QUALIFIER static char const * value() { return "b"; } };
  template<> struct lglmprefix<uint8_t> { GLM_FUNC_QUALIFIER static char const * value() { return "u8"; } };
  template<> struct lglmprefix<int8_t> { GLM_FUNC_QUALIFIER static char const * value() { return "i8"; } };
  template<> struct lglmprefix<uint16_t> { GLM_FUNC_QUALIFIER static char const * value() { return "u16"; } };
  template<> struct lglmprefix<int16_t> { GLM_FUNC_QUALIFIER static char const * value() { return "i16"; } };
  template<> struct lglmprefix<uint32_t> { GLM_FUNC_QUALIFIER static char const * value() { return "u"; } };
  template<> struct lglmprefix<int32_t> { GLM_FUNC_QUALIFIER static char const * value() { return "i"; } };
  template<> struct lglmprefix<uint64_t> { GLM_FUNC_QUALIFIER static char const * value() { return "u64"; } };
  template<> struct lglmprefix<int64_t> { GLM_FUNC_QUALIFIER static char const * value() { return "i64"; } };

  template<typename matType>
  struct lglm_compute_to_string { };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<vec<1, T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, vec<1, glm_Float> const &x) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%svec1(%s)", PrefixStr, LiteralStr);
      return _vsnprintf(buff, buff_len, format_text, GLM_STRING_CAST(x[0]));
    }
  };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<vec<2, T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, vec<2, glm_Float> const &x) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%svec2(%s, %s)", PrefixStr, LiteralStr, LiteralStr);
      return _vsnprintf(buff, buff_len, format_text, GLM_STRING_CAST(x[0]), GLM_STRING_CAST(x[1]));
    }
  };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<vec<3, T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, vec<3, glm_Float> const &x) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%svec3(%s, %s, %s)", PrefixStr, LiteralStr, LiteralStr, LiteralStr);
      return _vsnprintf(buff, buff_len, format_text,
        GLM_STRING_CAST(x[0]),
        GLM_STRING_CAST(x[1]),
        GLM_STRING_CAST(x[2])
      );
    }
  };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<vec<4, T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, vec<4, glm_Float> const &x) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%svec4(%s, %s, %s, %s)", PrefixStr, LiteralStr, LiteralStr, LiteralStr, LiteralStr);
      return _vsnprintf(buff, buff_len, format_text,
        GLM_STRING_CAST(x[0]),
        GLM_STRING_CAST(x[1]),
        GLM_STRING_CAST(x[2]),
        GLM_STRING_CAST(x[3])
      );
    }
  };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<qua<T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, qua<glm_Float> const &q) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%squat(%s, {%s, %s, %s})", PrefixStr, LiteralStr, LiteralStr, LiteralStr, LiteralStr);
      return _vsnprintf(buff, buff_len, format_text,
        GLM_STRING_CAST(q.w),
        GLM_STRING_CAST(q.x),
        GLM_STRING_CAST(q.y),
        GLM_STRING_CAST(q.z)
      );
    }
  };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<mat<2, 2, T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, mat<2, 2, T, Q> const &x) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%smat2x2((%s, %s), (%s, %s))",
        PrefixStr,
        LiteralStr, LiteralStr,
        LiteralStr, LiteralStr
      );

      return _vsnprintf(buff, buff_len, format_text,
        GLM_STRING_CAST(x[0][0]), GLM_STRING_CAST(x[0][1]),
        GLM_STRING_CAST(x[1][0]), GLM_STRING_CAST(x[1][1])
      );
    }
  };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<mat<2, 3, T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, mat<4, 3, T, Q> const &x) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%smat2x3((%s, %s, %s), (%s, %s, %s))",
        PrefixStr,
        LiteralStr, LiteralStr, LiteralStr,
        LiteralStr, LiteralStr, LiteralStr
      );

      return _vsnprintf(buff, buff_len, format_text,
        GLM_STRING_CAST(x[0][0]), GLM_STRING_CAST(x[0][1]), GLM_STRING_CAST(x[0][2]),
        GLM_STRING_CAST(x[1][0]), GLM_STRING_CAST(x[1][1]), GLM_STRING_CAST(x[1][2])
      );
    }
  };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<mat<2, 4, T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, mat<2, 4, T, Q> const &x) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%smat2x4((%s, %s, %s, %s), (%s, %s, %s, %s))",
        PrefixStr,
        LiteralStr, LiteralStr, LiteralStr, LiteralStr,
        LiteralStr, LiteralStr, LiteralStr, LiteralStr
      );

      return _vsnprintf(buff, buff_len, format_text,
        GLM_STRING_CAST(x[0][0]), GLM_STRING_CAST(x[0][1]), GLM_STRING_CAST(x[0][2]), GLM_STRING_CAST(x[0][3]),
        GLM_STRING_CAST(x[1][0]), GLM_STRING_CAST(x[1][1]), GLM_STRING_CAST(x[1][2]), GLM_STRING_CAST(x[1][3])
      );
    }
  };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<mat<3, 2, T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, mat<3, 2, T, Q> const &x) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%smat3x2((%s, %s), (%s, %s), (%s, %s))",
        PrefixStr,
        LiteralStr, LiteralStr,
        LiteralStr, LiteralStr,
        LiteralStr, LiteralStr
      );

      return _vsnprintf(buff, buff_len, format_text,
        GLM_STRING_CAST(x[0][0]), GLM_STRING_CAST(x[0][1]),
        GLM_STRING_CAST(x[1][0]), GLM_STRING_CAST(x[1][1]),
        GLM_STRING_CAST(x[2][0]), GLM_STRING_CAST(x[2][1])
      );
    }
  };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<mat<3, 3, T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, mat<3, 3, T, Q> const &x) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%smat3x3((%s, %s, %s), (%s, %s, %s), (%s, %s, %s))",
        PrefixStr,
        LiteralStr, LiteralStr, LiteralStr,
        LiteralStr, LiteralStr, LiteralStr,
        LiteralStr, LiteralStr, LiteralStr
      );

      return _vsnprintf(buff, buff_len, format_text,
        GLM_STRING_CAST(x[0][0]), GLM_STRING_CAST(x[0][1]), GLM_STRING_CAST(x[0][2]),
        GLM_STRING_CAST(x[1][0]), GLM_STRING_CAST(x[1][1]), GLM_STRING_CAST(x[1][2]),
        GLM_STRING_CAST(x[2][0]), GLM_STRING_CAST(x[2][1]), GLM_STRING_CAST(x[2][2])
      );
    }
  };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<mat<3, 4, T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, mat<3, 4, T, Q> const &x) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%smat3x4((%s, %s, %s, %s), (%s, %s, %s, %s), (%s, %s, %s, %s))",
        PrefixStr,
        LiteralStr, LiteralStr, LiteralStr, LiteralStr,
        LiteralStr, LiteralStr, LiteralStr, LiteralStr,
        LiteralStr, LiteralStr, LiteralStr, LiteralStr
      );

      return _vsnprintf(buff, buff_len, format_text,
        GLM_STRING_CAST(x[0][0]), GLM_STRING_CAST(x[0][1]), GLM_STRING_CAST(x[0][2]), GLM_STRING_CAST(x[0][3]),
        GLM_STRING_CAST(x[1][0]), GLM_STRING_CAST(x[1][1]), GLM_STRING_CAST(x[1][2]), GLM_STRING_CAST(x[1][3]),
        GLM_STRING_CAST(x[2][0]), GLM_STRING_CAST(x[2][1]), GLM_STRING_CAST(x[2][2]), GLM_STRING_CAST(x[2][3])
      );
    }
  };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<mat<4, 2, T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, mat<4, 2, T, Q> const &x) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%smat4x2((%s, %s), (%s, %s), (%s, %s), (%s, %s))",
        PrefixStr,
        LiteralStr, LiteralStr,
        LiteralStr, LiteralStr,
        LiteralStr, LiteralStr,
        LiteralStr, LiteralStr
      );

      return _vsnprintf(buff, buff_len, format_text,
        GLM_STRING_CAST(x[0][0]), GLM_STRING_CAST(x[0][1]),
        GLM_STRING_CAST(x[1][0]), GLM_STRING_CAST(x[1][1]),
        GLM_STRING_CAST(x[2][0]), GLM_STRING_CAST(x[2][1]),
        GLM_STRING_CAST(x[3][0]), GLM_STRING_CAST(x[3][1])
      );
    }
  };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<mat<4, 3, T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, mat<4, 3, T, Q> const &x) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%smat4x3((%s, %s, %s), (%s, %s, %s), (%s, %s, %s), (%s, %s, %s))",
        PrefixStr,
        LiteralStr, LiteralStr, LiteralStr,
        LiteralStr, LiteralStr, LiteralStr,
        LiteralStr, LiteralStr, LiteralStr,
        LiteralStr, LiteralStr, LiteralStr
      );

      return _vsnprintf(buff, buff_len, format_text,
        GLM_STRING_CAST(x[0][0]), GLM_STRING_CAST(x[0][1]), GLM_STRING_CAST(x[0][2]),
        GLM_STRING_CAST(x[1][0]), GLM_STRING_CAST(x[1][1]), GLM_STRING_CAST(x[1][2]),
        GLM_STRING_CAST(x[2][0]), GLM_STRING_CAST(x[2][1]), GLM_STRING_CAST(x[2][2]),
        GLM_STRING_CAST(x[3][0]), GLM_STRING_CAST(x[3][1]), GLM_STRING_CAST(x[3][2])
      );
    }
  };

  template<typename T, qualifier Q>
  struct lglm_compute_to_string<mat<4, 4, T, Q>> {
    static GLM_FUNC_QUALIFIER int call(char *buff, size_t buff_len, mat<4, 4, T, Q> const &x) {
      GLM_STRING_HEADER
      _vsnprintf(format_text, GLM_FORMAT_BUFFER, "%smat4x4((%s, %s, %s, %s), (%s, %s, %s, %s), (%s, %s, %s, %s), (%s, %s, %s, %s))",
        PrefixStr,
        LiteralStr, LiteralStr, LiteralStr, LiteralStr,
        LiteralStr, LiteralStr, LiteralStr, LiteralStr,
        LiteralStr, LiteralStr, LiteralStr, LiteralStr,
        LiteralStr, LiteralStr, LiteralStr, LiteralStr
      );

      return _vsnprintf(buff, buff_len, format_text,
        GLM_STRING_CAST(x[0][0]), GLM_STRING_CAST(x[0][1]), GLM_STRING_CAST(x[0][2]), GLM_STRING_CAST(x[0][3]),
        GLM_STRING_CAST(x[1][0]), GLM_STRING_CAST(x[1][1]), GLM_STRING_CAST(x[1][2]), GLM_STRING_CAST(x[1][3]),
        GLM_STRING_CAST(x[2][0]), GLM_STRING_CAST(x[2][1]), GLM_STRING_CAST(x[2][2]), GLM_STRING_CAST(x[2][3]),
        GLM_STRING_CAST(x[3][0]), GLM_STRING_CAST(x[3][1]), GLM_STRING_CAST(x[3][2]), GLM_STRING_CAST(x[3][3])
      );
    }
  };

  /// <summary>
  /// Return: the number of characters written if successful or negative value
  /// if an error occurred.
  /// </summary>
  template<class matType>
  static GLM_FUNC_QUALIFIER int format_type(char *buff, size_t buff_len, matType const &x) {
    return detail::lglm_compute_to_string<matType>::call(buff, buff_len, x);
  }
}
}

/* }================================================================== */

/*
** {==================================================================
** glm::hash implementation without std::hash dependency.
** ===================================================================
*/

namespace glm {
namespace hash {
#include <cmath>

#if !defined(l_hashfloat)
  /// <summary>
  /// ltable.c: l_hashfloat
  /// </summary>
  static GLM_FUNC_QUALIFIER int l_hashfloat(lua_Number n) {
    int i;
    lua_Integer ni = 0;
    n = std::frexp(n, &i) * -cast_num(INT_MIN);
    if (!lua_numbertointeger(n, &ni)) { /* is 'n' inf/-inf/NaN? */
      lua_assert(luai_numisnan(n) || l_mathop(fabs)(n) == cast_num(HUGE_VAL));
      return 0;
    }
    else { /* normal case */
      unsigned int u = cast_uint(i) + cast_uint(ni);
      return cast_int(u <= cast_uint(INT_MAX) ? u : ~u);
    }
  }
#endif

  static GLM_INLINE void hash_combine(size_t &seed, size_t hash) {
    hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= hash;
  }

  template<typename T, glm::qualifier Q>
  GLM_FUNC_QUALIFIER typename std::enable_if<std::is_floating_point<T>::value, size_t>::type hash(glm::vec<2, T, Q> const &v) {
    size_t seed = 0;
    hash_combine(seed, l_hashfloat(static_cast<lua_Number>(v.x)));
    hash_combine(seed, l_hashfloat(static_cast<lua_Number>(v.y)));
    return seed;
  }

  template<typename T, glm::qualifier Q>
  GLM_FUNC_QUALIFIER typename std::enable_if<std::is_floating_point<T>::value, size_t>::type hash(glm::vec<3, T, Q> const &v) {
    size_t seed = 0;
    hash_combine(seed, l_hashfloat(static_cast<lua_Number>(v.x)));
    hash_combine(seed, l_hashfloat(static_cast<lua_Number>(v.y)));
    hash_combine(seed, l_hashfloat(static_cast<lua_Number>(v.z)));
    return seed;
  }

  template<typename T, glm::qualifier Q>
  GLM_FUNC_QUALIFIER typename std::enable_if<std::is_floating_point<T>::value, size_t>::type hash(glm::vec<4, T, Q> const &v) {
    size_t seed = 0;
    hash_combine(seed, l_hashfloat(static_cast<lua_Number>(v.x)));
    hash_combine(seed, l_hashfloat(static_cast<lua_Number>(v.y)));
    hash_combine(seed, l_hashfloat(static_cast<lua_Number>(v.z)));
    hash_combine(seed, l_hashfloat(static_cast<lua_Number>(v.w)));
    return seed;
  }

  template<typename T, glm::qualifier Q>
  GLM_FUNC_QUALIFIER typename std::enable_if<std::is_floating_point<T>::value, size_t>::type hash(glm::qua<T, Q> const &v) {
    size_t seed = 0;
    hash_combine(seed, l_hashfloat(static_cast<lua_Number>(v.x)));
    hash_combine(seed, l_hashfloat(static_cast<lua_Number>(v.y)));
    hash_combine(seed, l_hashfloat(static_cast<lua_Number>(v.z)));
    hash_combine(seed, l_hashfloat(static_cast<lua_Number>(v.w)));
    return seed;
  }
}
}

/* }================================================================== */

#undef GLM_STRING_HEADER

#endif
