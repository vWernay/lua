**Automatically Generated**
# Extended Functions:
## Constructors
All functions declared in the global table (`_G`).
### vec
```lua
-- Generic vector constructor; infers dimensionality based on the number of
-- numeric arguments.
--
-- Rules:
--   1. A primitive type (float, int, bool) will be stored at v[X];
--   2. A vector/quaternions (of N dimensions) will have its contents stored
--      at v[X], v[X + 1], ..., v[X + N] following x, y, z, w ordering;
--   3. Otherwise, a lua_error is thrown.
vector = vec(...)

-- Applies integer-casting rules to the input values
ivec = ivec(...)

-- Applies bool-casting to the input values
bvec = bvec(...)

--[[
    vec#(value) - Create a vector of specific dimensionality with the value
      for each dimension.

    vec#(...) that ensures two, three, four numeric arguments respectively:
      - vec2(...) / ivec2(...) / bvec2(...)
      - vec3(...) / ivec3(...) / bvec3(...)
      - vec4(...) / ivec4(...) / bvec4(...)
--]]
```
#### Examples
```lua
> vec(math.pi, math.pi, math.pi)
vec3(3.141593, 3.141593, 3.141593)

> vec3(math.pi)
vec(3.141593, 3.141593, 3.141593)

-- Creates an integer vector (note: vectors are floating point internally)
> ivec3(1.1,2.2,3.8)
vec3(1.000000, 2.000000, 3.000000)
```

### mat
```lua
-- Generic matrix population/construction function: this function will iterate
-- over the current Lua stack, expecting a component vector for each dimension
-- of the matrix. If there is only one argument passed to the function and it
-- is an array, it will be iterated over instead.
--
-- Note: This function will call vec(...) for each column.
matrix = mat(...)

-- Matrix constructor that recycles p_matrix.
matrix = mat(p_matrix, ...)

--[[
    Ensures dimensionality based on the "NxM" suffix (each accepting an optional
    p_matrix):
    mat2x2(...), mat2x3(...), mat2x4(...),
    mat3x2(...), mat3x3(...), mat3x4(...),
    mat4x2(...), mat4x3(...), mat4x4(...)
--]]
```

#### Examples
```lua
> mat3x3(vec3(glm.e, glm.e, glm.e), vec3(math.pi, math.pi, math.pi), vec3(1,1,1))
mat3x3((2.718282, 2.718282, 2.718282), (3.141593, 3.141593, 3.141593), (1.000000, 1.000000, 1.000000))
```

### qua
```lua
-- Generic quaternion constructor. Note, "qua" also aliased to quat.
q = quat(w --[[ number ]], x --[[ number ]], y --[[ number ]], z --[[ number ]])
q = quat(xyz --[[ vec3 ]], w --[[ number ]])

-- The shortest arc quaternion that rotates a source direction to coincide with
-- the target.
q = quat(source --[[ vec3 ]], target --[[ vec3 ]])

-- Build a quaternion from an angle (in degrees for grit-lua compatibility) and
-- a normalized axis.
q = quat(angle --[[ number ]], axis --[[ vec3 ]])

-- matrix to quaternion.
q = quat(m --[[ mat3x3 ]])
q = quat(m --[[ mat4x4 ]])
```

## grit-lua compatibility
Compatibility functions declared in the global table (`_G`):

### dot
```lua
-- Returns the dot product of x and y.
integer = dot(x --[[ integer ]], y --[[ integer ]])
number = dot(x --[[ number ]], y --[[ number ]])
number = dot(x --[[ vecN ]], y --[[ vecN ]])
number = dot(x --[[ quat ]], y --[[ quat ]])
```

### cross
```lua
-- Returns the cross product of x and y.
number = cross(x --[[ vec2 ]], y --[[ vec2 ]])
vec3 = cross(x --[[ vec3 ]], y --[[ vec3 ]])
vec3 = cross(x --[[ vec3 ]], y --[[ quat ]])
vec3 = cross(x --[[ quat ]], y --[[ vec3 ]])
quat = cross(x --[[ quat ]], y --[[ quat ]])
```

### inverse
```lua
-- Returns the quaternion inverse.
quat = inverse(q --[[ quat ]])

-- Return the inverse of a squared matrix.
mat2x2 = inverse(m --[[ mat2x2 ]])
mat3x3 = inverse(m --[[ mat3x3 ]])
mat4x4 = inverse(m --[[ mat4x4 ]])
```

### norm
```lua
-- Returns a vector in the same direction as x but with length of 1.
vecN = norm(v --[[ vecN ]])
quat = norm(q --[[ quat ]])
```

### slerp
```lua
-- Returns spherical interpolation between two vectors.
vecN = slerp(x --[[ vecN ]], y --[[ vecN ]], t --[[ number ]])
quat = slerp(x --[[ quat ]], y --[[ quat ]], t --[[ number ]])
```
