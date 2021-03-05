# LuaGLM
A Lua runtime providing vector, quaternion, and matrix data types with a (near-)complete binding to [GLM 0.9.9.9](https://github.com/g-truc/glm), a C++ mathematics library based on the [OpenGL Shading Language (GLSL)](https://www.opengl.org/registry/doc/GLSLangSpec.4.50.diff.pdf) specifications.

> A significantly less efficient shared-library implementation, using tables and/or userdata instead of first-class types, for Lua5.1, Lua5.2, Lua5.3, Lua5.4, and [LuaJIT](https://github.com/LuaJIT/LuaJIT), can be found [TBD](https://github.com/gottfriedleibniz/...).

## Vectors/Quaternions
Vectors and quaternions are first-class entities and are viewed (both syntactically and semantically) as **immutable** tables of floats and only accessible by keys `1, 2, 3, 4`, `x, y, z, w`, and `r, g, b, a`:
```lua
-- Generic vector constructor;
> v = vec(math.pi, math.exp(1), (1 + math.sqrt(5)) / 2)

-- Vector constructor with explicit length;
> v3 = vec3(math.pi, math.exp(1), (1 + math.sqrt(5)) / 2)

-- Access the vectors fields;
> v[3] + v.z
3.2360680103302

-- Vector swizzling;
> v.xyzx
vec4(3.141593, 2.718282, 1.618034, 3.141593)

-- vectors and quaternions have an explicit type string even though they are
-- internally represented by the same LUA_TVECTOR tag (grit-lua compatbility);
> print(type(v), type(quat(1,0,0,0)))
vector3 quat

-- Test vectors for equality;
> v == v3
true

-- Arithmetic operators on vector types;
> ((v + v3) * v) - v3
vec3(16.597618, 12.059830, 3.618034)

-- The length operator returns the magnitude of the vector;
> #v
4.4583287239075

-- Vector dimensionality;
> v.n
3

-- Iterate over each component with 'pairs';
> for k,v in pairs(v3) do print(k,v) end
x       3.1415927410126
y       2.7182817459106
z       1.6180340051651

-- Create a quaternion by rotating an axis by 35 degrees;
> quat(35.0, vec(1,0,0))
quat(0.953717, {0.300706, 0.000000, 0.000000})

-- Multiply a direction vector by the quaternion;
> quat(35.0, vec(1,0,0)) * norm(vec(1,1,1))
vec3(0.577350, 0.141783, 0.804092)

-- Use vectors as table keys.
> t = { }
> t[v // 1.0] = "Hello, World!"
> t[v // 1.0]
Hello, World!
```

When vectors/quaternions are accessed by other values some additional rules exist prior to a `__index` metamethod lookup:
1. If the length of a string key is less than or equal to four characters, it is first passed through a swizzling filter, e.g., `v.zyx == vec3(v.z, v.y, v.x)`, returning a vector if all characters are valid fields;
1. For quaternions, the `angle` and `axis` strings are reserved for the angle (in degrees) and normalized axis of rotation (grit-lua compatibility);
1. The dimensionality of a vector can be accessed by the `n` and `dim` strings as the length operator returns the vector magnitude (grit-lua compatibility).

Vector and quaternion types do not maintain an explicit metatable reference. The Lua functions `getmetatable` and `debug.setmetatable`, and C API functions `lua_setmetatable` and `lua_getmetatable` can be used to define explicit metatables for these types.

#### C API
Vector and quaternions values are represented by the `LUA_TVECTOR` tag. They are primitive types (immutable) and use a struct of floats to represent each component. On an API level, vectors and quaternions are effectively tables and accessing their values can be done using the same Lua API functions:
- [`lua_rawget`](https://www.lua.org/manual/5.4/manual.html#lua_rawget);
- [`lua_rawgeti`](https://www.lua.org/manual/5.4/manual.html#lua_rawgeti);
- [`lua_geti`](https://www.lua.org/manual/5.4/manual.html#lua_geti);
- [`lua_getfield`](https://www.lua.org/manual/5.4/manual.html#lua_getfield);
- [`lua_rawlen`](https://www.lua.org/manual/5.4/manual.html#lua_rawlen): Returns the dimensionality of the vector;
- [`lua_len`](https://www.lua.org/manual/5.4/manual.html#lua_len): Pushes the magnitude of the vector at the given index onto the stack (grit-lua compatibility).

See [lglm.hpp](lglm.hpp), the external header for interfacing with ``glm`` defined structures within Lua. The deprecated grit-lua API can be referenced by [lgrit_lib.h](lgrit_lib.h).

## Matrices
Matrices are **mutable** collections of **column**(-major) vectors that are only accessible by keys `1, 2, 3, 4`. They are **collectible** objects and beholden to the garbage collector.

```lua
-- Create a matrix;
> m = mat(vec(1.0, 0.0, 0.0), vec(0.0, 0.819152, 0.573576), vec(0.0, -0.573576, 0.819152))

-- matrices are first-class types;
> type(m)
matrix

-- tostring for matrix and vector types invokes glm::to_string;
> tostring(m)
mat3x3((1.000000, 0.000000, 0.000000), (0.000000, 0.819152, 0.573576), (0.000000, -0.573576, 0.819152))

-- The length operator corresponds to the number of column vectors;
> #m
3

-- Access a column component;
> m[2]
vec3(0.000000, 0.819152, 0.573576)

-- Iterate over each matrix component;
> for k,v in pairs(m) do print(k, v) end
1       vec3(1.000000, 0.000000, 0.000000)
2       vec3(0.000000, 0.819152, 0.573576)
3       vec3(0.000000, -0.573576, 0.819152)

-- Multiply a vector by the given matrix;
> m * vec(0,1,0)
vec3(0.000000, 0.819152, 0.573576)

-- Append a column vector;
> m[4] = vec3(0)
> m
mat4x3((1.000000, 0.000000, 0.000000), (0.000000, 0.819152, 0.573576), (0.000000, -0.573576, 0.819152), (0.000000, 0.000000, 0.000000))

-- Remove column vectors. Note only m[#m] can be removed at a time;
> m[3],m[4] = nil,nil
> m
mat2x3((1.000000, 2.000000, 3.000000), (4.000000, 5.000000, 6.000000))
```

#### C API
Matrices are objects and represented by the `LUA_TMATRIX` tag. On an API level, they are effectively tables (more specifically arrays) and accessing/modifying their components can be done with same Lua API functions:
- [`lua_gettable`](https://www.lua.org/manual/5.4/manual.html#lua_gettable), [`lua_settable`](https://www.lua.org/manual/5.4/manual.html#lua_settable);
- [`lua_geti`](https://www.lua.org/manual/5.4/manual.html#lua_geti), [`lua_seti`](https://www.lua.org/manual/5.4/manual.html#lua_seti);
- [`lua_rawget`](https://www.lua.org/manual/5.4/manual.html#lua_rawget), [`lua_rawset`](https://www.lua.org/manual/5.4/manual.html#lua_rawset);
- [`lua_rawgeti`](https://www.lua.org/manual/5.4/manual.html#lua_rawgeti), [`lua_rawseti`](https://www.lua.org/manual/5.4/manual.html#lua_rawseti).
- [`lua_rawlen`](https://www.lua.org/manual/5.4/manual.html#lua_rawlen), [`lua_len`](https://www.lua.org/manual/5.4/manual.html#lua_len): Returns the dimensionality (number of columns) of the matrix;

See [lglm.hpp](...), the external header for interfacing with ``glm`` defined matrices within Lua.

## Power Patches
This runtime [imports](http://lua-users.org/wiki/LuaPowerPatches) many (small) useful changes to the Lua runtime, all bound to preprocessor flags:

#### Compound Operators:
Add ``+=, -=, *=, /=, <<=, >>=, &=, |=, and ^=`` to the language. The increment and decrement operators (``++, --``) have not been implemented due to one of those operators being reserved.

#### Safe Navigation:
An indexing operation that suppresses errors on accesses into undefined tables (similar to the safe-navigation operators in C#, Kotlin, etc.), e.g.,

```lua
t?.x?.y == nil
```

#### In Unpacking:
Support for unpacking named values from tables using the ``in`` keyword, e.g,

```lua
local a,b,c in t
```

is functionally equivalent to:

```lua
local a,b,c = t.a,t.b,t.c
```

#### Set Constructors:
Syntactic sugar to improve the syntax for specifying sets, i.e.,

```lua
t = { .a, .b }
```

is functionally equivalent to:

```lua
t = { a = true, b = true }
```

#### C-Style Comments:
Support for C-style block comments: ``/* Comment */``, e.g.,

```lua
print("Hello, World!") /* Comment */
```

#### Parameterless Do:
Syntactic sugar for ``function() ... end`` statements, e.g.,

```lua
do --[[ CodeBlock ]] end
```

is functionally equivalent to:

```lua
function() --[[ CodeBlock ]] end
```

#### Compile Time Jenkins' Hashes:
String literals wrapped in back-ticks are Jenkins' one-at-a-time hashed when parsed.

```lua
> `Hello, World!`
1395890823
```

For run-time hashing, the `joaat` function is included in the base library:

```lua
-- joaat(input, [, ignore_casing]): Compute the Jenkins hash of the input string.
-- If 'ignore_casing' is true, the byte data is hashed as is. Otherwise, each
-- ASCII character is tolower'd prior to hashing.
> joaat("Hello, World!")
1395890823
```

Note: for compatibility reasons, all hashes returned are sign-extended:

```lua
> joaat("CPed")
-1803413927

> joaat("CPed") & 0xFFFFFFFF
2491553369
```

#### \_\_ipairs:
Reintroduce compatibility for the ``__ipairs`` metamethod that was deprecated in 5.3 and removed in 5.4.

#### Defer:
Import ``func2close`` from ltests.h into the base library. Initially, Lua 5.4 was designed so that functions could be treated as to-be-closed variables. However, that feature interacts poorly with sandboxing and coroutines (see: [No more to-be-closed functions](https://github.com/lua/lua/commit/4ace93ca6502dd1da38d5c06fa099d229e791ba8)).

```lua
-- closing function. Could also be used to supply a to-be-closed variable to a
-- generic for loop
local _ <close> = defer(function() numopen = numopen - 1 end)
```

#### Each Iteration:
In Lua 5.4, a generic 'for' loop starts by evaluating its explist to produce four values: an iterator function, a state, an initial value for the control variable, and a closing (to-be-closed) value. However, the \_\_pairs metamethod does not support the optional to-be-closed variable.

This extension introduces a ``__iter`` metamethod to allow ``next, t, nil, to-be-closed`` forms of iteration bound to a metamethod. To iterate over such a table, only the table needs to be supplied as an argument to a loop, e.g.,

```lua
t = {1,2,3}
for k,v in each(t) do print(k, v) end
```

which defaults to a ``pairs`` implementation that supports the fourth return variable (to-be-closed) when no `__iter` metamethod exists. ... This patch is inspired by the Self-iterating Objects patch; see commit logs for reasoning behind deviation.

#### String Blobs
Introduce a `LUA_TSTRING` variant that is effectively a `LUA_VLNGSTR` but without the hash caching mechanism. Values of this variant are stored in tables by reference.

```lua
-- Create a string blob of specified length;
blob = string.blob(512)

-- Create a blob variant of another string;
blob = string.blob(string.rep('\0', 80))

-- Returns a string blob containing the values v1, v2, etc. serialized in binary
-- form (packed at an optional offset) according to the format string fmt.
--
-- Unlike string.pack, this function attempts to write the resulting
-- serialization onto the provided blob (at an optional offset). If the blob is
-- not of sufficient size, a (byte-)copy of the blob is made and returned.
_blob = string.blob_pack(blob, pos --[[ optional ]], fmt, v1, v2, ···)

-- An alias to string.unpack
... = string.blob_unpack (fmt, s [, pos])
```

With included C API functions:

```c
/*
** Creates a string blob of at least "len" bytes, pushing the zero-terminated
** blob onto the stack. Returning a pointer to that blob inside the Lua state.
*/
const char *lua_pushblob(lua_State *L, size_t len);

/*
** Converts the (explicit) string at the given index to a blob. If the string
** value is not already a blob, then lua_tostringblob changes the actual value
** in the stack to a blob (same lua_tolstring caveats apply).
*/
const char *lua_tostringblob(lua_State *L, int idx, size_t *len);
```

At some point an API similar to [DataView](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/DataView) that interface with Luas built in facilities, e.g., string.pack, string.unpack, and table.concat, will be implemented.

The current solution simply allows byte data to still be beholden to the garbage collector while not requiring the allocation of intermediate data when going to and from the Lua API.

#### Extended API:
Expose ``lua_createtable`` and API functions common to other Lua runtimes.

```lua
-- Creates a new empty table
-- narr: a hint for how many elements the table will have as a sequence.
-- nrec: a hint for how many other elements the table will have.
t = table.create(narr, nrec)

-- Restore the table to its initial value (removing its contents) while
-- retaining its internal pointer;
t = table.wipe(t)

-- An efficient (implemented using memcpy) table shallow-copy implementation;
t2 = table.clone(t)

-- Return the type of table being used. Note, this function only measures the
-- size of the "array part" of a Lua table and the "root" node of its
-- "hash part". Once an "array" becomes "mixed", or if a table has all of
-- values nil'd out, the table.type will remain "mixed" or "hash".
label = table.type(t) -- "empty", "array", "hash", or "mixed"

-- Joins strings together with a delimiter;
str = string.strjoin(delimiter [, string, ...])

-- Trim characters off the beginning and end of a string;
str = string.strtrim(input [, chars])

-- Returns a concatenation of all number/string arguments passed;
str = string.strconcat(...)

-- Splits a string using a delimiter (optionally: into a specified number of pieces);
... = string.strsplit(delimiter [, string [, pieces]])

-- Converts all arguments to strings;
... = string.tostringall(...)

-- Alias: Returns the number of UTF-8 characters in string s;
utf8.strlenutf8 = utf8.len

-- String comparison accounting for UTF-8 chars. Returning a negative integer,
-- zero, or a positive integer if the first string is less than, equal to, or
-- greater than the second string;
result = utf8.strcmputf8i(stringLH, stringRH)

--- Return all arguments with non-number/boolean/string values changed to nil;
... = scrub(...)

```

#### Library Preloading:
Allow the preloading of libraries, e.g., ``lib = require('...')``, via command line arguments.
```bash
# Preload a JSON library (stored at _G.rapidjson) and msgpack library (stored at _G.msgpack)
./lua -lglm -lrapidjson -lmsgpack=cmsgpack
```

#### Readline History:
Keep a persistent list of commands that have been run on the Lua interpreter. With the `LUA_HISTORY` environment variable used to declare the location history.

## Building
The Lua core can be compiled as C or as C++ code. All functions required to integrate GLM into Lua are defined in [lglm_core.h](lglm_core.h), [lglm.hpp](lglm.hpp) provides the mechanism for aliasing the GLM structures across C/C++ boundaries, and the GLM/Lua integration is implemented in [lglm.cpp](lglm.cpp).

As `lglm.cpp` must be compiled as C++, the preprocessor flag `LUA_C_LINKAGE` is used to synchronize the calling convention required to link it with the rest of Lua. Beware of language boundary issues!

### Make
An modified version of the makefile [bundled](https://www.lua.org/download.html) with release versions of Lua. The same instructions apply:
```bash
# Build for the linux platform target, linked with readline;
└> make linux-readline

# Run Lua
└> ./lua
```

### CMake
A CMake project that builds the stand-alone interpreter (`lua`), a compiler (`luac`), and shared/static libraries. This project includes variables for most preprocessor configuration flags supported by Lua and GLM. See `cmake -LAH` or [cmake-gui](https://cmake.org/runningcmake/) for the complete list of build options.

```bash
# Create build directory
└> mkdir -p build ; cd build

# Compile with icpx: -DCMAKE_CXX_COMPILER=/opt/intel/[...]/bin/icpx
└> cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release ..

# Build
└> make
```

Note: for msbuild environments, it's preferred to the the LLVM toolset (`-T llvm`).

### Lua Preprocessor Configurations
Note, not all Lua-specific options are listed.

- **Compilation**:
  - **ONE\_LUA**: Compile Lua core, libraries, and interpreter as a single file;
  - **LUA\_C\_LINKAGE**: An indication to `lglm.cpp` that has C linkage;
  - **LUA\_NATIVE\_ARCH**: Enable compiler optimizations for the native processor architecture;
  - **LUA\_NO\_DUMP**: Disable the dump module (dumping Lua functions as precompiled chunks);
  - **LUA\_NO\_BYTECODE**: Disables the usage of lua_load with binary chunks;
  - **LUA\_NO\_PARSER**: Compile the Lua core so it does not contain the parsing modules (lcode, llex, lparser). Only binary files and strings, precompiled with luac, can be loaded.
- **Testing**:
  - **LUA\_INCLUDE\_TEST**: Include ltests.h and testing modules (this option enables many of the following flags by default);
  - **LUAI\_ASSERT**: Turn on all assertions inside Lua;
  - **LUA\_USE\_APICHECK**: Turns on several consistency checks on the C API;
  - **HARDSTACKTESTS**: Force a reallocation of the stack at every point where the stack can be reallocated;
  - **HARDMEMTESTS**: Force a full collection at all points where the collector can run;
  - **EMERGENCYGCTESTS**: Force an emergency collection at every single allocation;
  - **EXTERNMEMCHECK**: Removes internal consistency checking of blocks being deallocated.
- **Power Patches**: See Lua Power Patches section.
  - **GRIT\_POWER\_COMPOUND**
  - **GRIT\_POWER\_SAFENAV**
  - **GRIT\_POWER\_INTABLE**
  - **GRIT\_POWER\_TABINIT**
  - **GRIT\_POWER\_CCOMMENT**
  - **GRIT\_POWER\_ANONDO**
  - **GRIT\_POWER\_JOAAT**
  - **GRIT\_COMPAT\_IPAIRS**
  - **GRIT\_DEFER**
  - **GRIT\_POWER\_EACH**
  - **GRIT\_POWER\_BLOB**
  - **GRIT\_POWER\_WOW**
  - **GRIT\_POWER\_PRELOADLIBS**
  - **GRIT\_POWER\_READLINE\_HISTORY**

#### GLM Preprocessor Configurations:
- **GLM\_FORCE\_MESSAGES**: Platform auto detection and default configuration;
- **GLM\_FORCE\_INLINE**: Force inline;
- **GLM\_FORCE\_ALIGNED\_GENTYPES**: Force GLM to enable aligned types;
- **GLM\_FORCE\_DEFAULT\_ALIGNED\_GENTYPES**: Force GLM to use aligned types by default;
- **GLM\_FORCE\_INTRINSICS**: Using SIMD optimizations;
- **GLM\_FORCE\_PRECISION\_**: Default precision;
- **GLM\_FORCE\_SINGLE\_ONLY**: Removed explicit 64-bits floating point types;
- **GLM\_FORCE\_XYZW\_ONLY**: Only exposes x, y, z and w components;
- **GLM\_FORCE\_LEFT\_HANDED**: Force left handed coordinate system;
- **GLM\_FORCE\_DEPTH\_ZERO\_TO\_ONE**: Force the use of a clip space between 0 to 1;
- **GLM\_FORCE\_SIZE\_T\_LENGTH**: Vector and matrix static size type;
- **GLM\_FORCE\_UNRESTRICTED\_GENTYPE**: Removing genType restriction;
- **GLM\_FORCE\_SILENT\_WARNINGS**: Silent C++ warnings from language extensions;
- **GLM\_FORCE\_QUAT\_DATA\_WXYZ**: Force GLM to store quat data as w,x,y,z instead of x,y,z,w.

For all GLM preprocessor flags, reference the [GLM manual](https://github.com/g-truc/glm/blob/master/manual.md#section2).

#### Added GLM Preprocessor Configurations:
- **GLM\_FAST\_MATH**: Enable fast math optimizations (see `-ffast-math` caveats);

## Developer Notes:

#### Planned Features:
1. One downside to vectors/quaternions being an explicit `Value` is that they increase the minimum `Value` to at least 16 bytes. Given that types in Lua are fairly transparent, it may be beneficial to introduce, or at least experiment with, a compile-time option to make vector/quaternion types collectible;
1. Support for integer vectors/matrices. Either by introducing an additional type, e.g., `LUA_TVECTORI`, or splitting the vector tag `LUA_TVECTOR` into `LUA_TVECTOR2`, `LUA_TVECTOR3`, `LUA_TVECTOR4`, and `LUA_TQUAT` and use variant bits for the primitive type;
1. A LINQ-style library that takes advantage of `__iter/__pairs`;

#### Tweaks:
1. Fix/improve MSVC portions of CMakeLists;
1. `glmMat_set` support for tables, e.g., `mat[i] = { ... }`, by using `glmH_tovector`;
1. Improve support for `glm::mat3x4` and `glm::mat4x3`;

## Benchmarking
**TODO**: Finish comparisons to...
- Lua/[LuaJIT](https://github.com/LuaJIT/LuaJIT) + [excessive/cpml](https://github.com/excessive/cpml)
- [NumPy](https://github.com/numpy/numpy)
- [PyGLM](https://github.com/Zuzu-Typ/PyGLM)
- [Unity.Mathematics](https://github.com/Unity-Technologies/Unity.Mathematics)

## Sources & Acknowledgments:
1. [grit-lua](https://github.com/grit-engine/grit-lua): Original implementation (and inspiration);
1. [SharpDX](https://github.com/sharpdx/SharpDX): Reference for Extended API (aliases);
1. [Unity](https://docs.unity3d.com/ScriptReference/UnityEngine.CoreModule.html): Reference for Extended API (aliases and emulation of some [mathf](https://docs.unity3d.com/ScriptReference/Mathf.html) functions).

## License
Lua and GLM are distributed under the terms of the [MIT license](https://opensource.org/licenses/mit-license.html). See the Copyright Notice in [lua.h](lua.h).
