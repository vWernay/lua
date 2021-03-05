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

## Constants
See [glm/gtc/constants.hpp][http://glm.g-truc.net/0.9.9/api/a00708.html]:
- `feps`: vector-float epsilon;
- `huge`: lmathlib compatibility;
- `maxinteger`: lmathlib compatibility;
- `mininteger`: lmathlib compatibility;
- `FP_NORMAL, FP_SUBNORMAL, FP_ZERO, FP_INFINITE, FP_NAN`: [Floating Point Categories][https://en.cppreference.com/w/cpp/numeric/math/FP_categories];

## Base:
### hash
```lua
-- std::hash wrapper
integer = hash(v --[[ vecN ]])
integer = hash(q --[[ quat ]])
integer = hash(m --[[ matNxM ]])
```


### unpack
```lua
-- Unpack the components of the provided type.
--
-- Vector: Push the available x, y, z, w fields.
-- Matrix: Push each column (or row) vector.
... = unpack(v --[[ vecN ]])
... = unpack(q --[[ quat ]])
... = unpack(m --[[ matNxM ]])
```

### to\_string
```lua
-- glm::to_string wrapper
string = to_string(v --[[ vecN ]])
string = to_string(q --[[ quat ]])
string = to_string(m --[[ matNxM ]])
```

### right
```lua
-- A unit vector designating right
vec3 = right()
```

### up
```lua
-- A unit vector designating up, see GLM_FORCE_Z_UP
vec3 = up()
```

### forward
```lua
-- A unit vector designating forward, see GLM_FORCE_Z_UP and GLM_FORCE_LEFT_HANDED
vec3 = forward()

-- A unit vector designating forward for left-handed coordinate systems,
-- however, still dependent on the state of GLM_FORCE_Z_UP
vec3 = forwardLH()

-- A unit vector designating forward for right-handed coordinate systems
vec3 = forwardRH()
```

## glm/common.hpp:
### signP
```lua
-- Returns the non-negative sign (1.0 if >= 0) for each component
s --[[ number ]] = signP(x --[[ number ]])
s --[[ vecN ]] = signP(x --[[ vecN ]])
```

### signN
```lua
-- Returns the non-positive sign (1.0 if > 0) for each component
s --[[ number ]] = signN(x --[[ number ]])
s --[[ vecN ]] = signN(x --[[ vecN ]])
```

### fdim
```lua
-- Returns the positive difference between x and y (C99/C++11)
vecN = fdim(x --[[ vecN ]], y --[[ vecN ]])
```

### hypot
```lua
-- Returns the hypotenuse of a right-angled triangle whose legs are x and y
-- (C99/C++11)
vecN = hypot(x --[[ vecN ]], y --[[ vecN ]])
```

### isnormal
```lua
-- Returns whether x is a normal value: i.e., whether it is neither
-- infinity, NaN, zero, or subnormal (C99/C++11)
bvecN = isnormal(x --[[ vecN ]])
```

### isunordered
```lua
-- Returns whether x or y are unordered values (C99/C++11)
bvecN = isunordered(x --[[ vecN ]], y --[[ vecN ]])
```

### nearbyint
```lua
-- Rounds x to an integral value (C99/C++11)
vecN = nearbyint(x --[[ vecN ]])
```

### nextafter
```lua
-- Returns the next representable value after x in the direction of y
-- (C99/C++11)
vecN = nextafter(x --[[ vecN ]], y --[[ vecN ]])
```

### remainder
```lua
-- Returns the floating-point remainder of numer/denom (C99/C++11)
vecN = remainder(numer --[[ vecN ]], denom --[[ vecN ]])
```

### scalbn
```lua
-- Scales x by FLT_RADIX raised to the power of n (C99/C++11)
vecN = scalbn(x --[[ vecN ]], n --[[ ivecN ]])
```

### copysign
```lua
-- Returns a value with the magnitude of x and the sign of y (C99/C++11)
vecN = copysign(x --[[ vecN ]], y --[[ vecN ]])
```

### fpclassify
```lua
-- Returns a value of type int that matches one of the classification macro
-- constants, depending on the value of x (C99/C++11)
ivecN = fpclassify(x --[[ vecN ]])
```

### Aliases:
- `fabs = abs`
- `tointeger = toint`
- `signbit = sign`

## glm/exponential.hpp:
### expm1
```lua
-- Returns e raised to the power x minus one: ex-1 (C99/C++11)
vecN = expm1(v --[[ vecN ]])
```

### cbrt
```lua
-- Returns the cubic root of x (C99/C++11)
vecN = cbrt(v --[[ vecN ]])
```

### log10
```lua
-- Returns the common (base-10) logarithm of x (C99/C++11)
vecN = log10(v --[[ vecN ]])
```

### log1p
```lua
-- Returns the natural logarithm of one plus x (C99/C++11)
vecN = log1p(v --[[ vecN ]])
```

### logb
```lua
-- Returns the logarithm of |x|, using FLT_RADIX as base for the logarithm
-- (C99/C++11)
vecN = logb(v --[[ vecN ]])
```

### ilogb
```lua
-- Returns the integral part of the logarithm of |x|, using FLT_RADIX as base
-- for the logarithm (C99/C++11)
ivecN = ilogb(v --[[ vecN ]])
```

## glm/trigonometric.hpp:
### sincos
```lua
-- Calculate sin and cos simultaneously
nsin --[[ number ]],ncos --[[ number ]] = sincos(v --[[ number ]])
vsin --[[ vecN ]],vcos --[[ vecN ]] = sincos(v --[[ vecN ]])
```

### Aliases:
- `deg = degrees`
- `rad = radians`

## glm/matrix.hpp:
### invertible
```lua
-- Return true if the matrix has an inverse (up to a given epsilon)
bool = invertible(m --[[ matNxM ]])
```

## glm/vector\_relational.hpp:
### ult
```lua
-- Unsigned x < y (lmathlib)
bvecN = ult(x --[[ vecN ]], y --[[ vecN ]])
```

### ulte
```lua
-- Unsigned x <= y (lmathlib)
bvecN = ulte(x --[[ vecN ]], y --[[ vecN ]])
```

## glm/geometric.hpp
### clampLength
```lua
-- Return a copy of the vector "v" with its length clamped to "maxLength"
vecN = clampLength(v --[[ vecN ]], maxLength --[[ number ]])
```

### scaleLength
```lua
-- Scale the length of vector "v" to a newLength
vecN = scaleLength(v --[[ vecN ]], newLength --[[ number ]])
```

### Aliases:
- `norm = normalize`
- `magnitude = length`
- `clampMagnitude = clampLength`
- `scaleMagnitude = scaleLength`

## glm/functions.hpp:
### moveTowards
```lua
-- Return a position between two points, moving no further from "from" than a
-- specified distance
vecN = moveTowards(
    from --[[ vecN ]],
    to --[[ vecN ]],
    maxDistance --[[ number ]]
)
```

### rotateTowards
```lua
-- Return a direction between two vectors; rotating no further from "from" than
-- a specified angle
vecN = rotateTowards(
    from --[[ vecN ]],
    to --[[ vecN ]],
    maxRadians --[[ number ]],
    maxLength --[[ number ]]
)
```

### smoothDamp
```lua
-- Gradually changes a vector position to reach the target position over time.
-- This calculation is derived from a smoothing time (an approximate time to
-- reach the target position) and an optional maximum speed
newPos --[[ vecN ]], newVelo --[[ vecN ]] = smoothDamp(
    currentPos --[[ vecN ]],
    targetPos --[[ vecN ]],
    currentVelo --[[ vecN ]],
    smooth_time --[[ number ]],
    maxSpeed --[[ number ]],
    deltaTime --[[ number ]]
)
```

### erf
```lua
-- Returns the error function value (C99/C++11)
vecN = erf(v --[[ vecN ]])
```

### erfc
```lua
-- Returns the complementary error function value (C99/C++11)
vecN = erfc(v --[[ vecN ]])
```

### lgamma
```lua
-- Returns the natural logarithm of the absolute value of the gamma function
-- (C99/C++11)
vecN = lgamma(v --[[ vecN ]])
```

### tgamma
```lua
-- Returns the gamma function (C99/C++11)
vecN = tgamma(v --[[ vecN ]])
```

## glm/ext/matrix\_projection.hpp:
### rayPicking
```lua
-- Note, mouse coordinates are on a [-1, 1] scale
vec3 = rayPicking(
    cam_direction --[[ vec3 ]],
    cam_up --[[ vec3 ]],
    fov --[[ number ]],
    aspectRatio --[[ number ]],
    nearDof --[[ number ]],
    farDof --[[ number ]],
    mouseX --[[ number ]],
    mouseY --[[ number ]]
)
```

### containsProjection
```lua
-- Returns true if the matrix contains a projection, i.e., the last row differs
-- (up to an epsilon) of [0, 0, 0, 1]
bool = containsProjection(m --[[ mat4x4 ]], epsilon --[[ number ]])
```

## glm/ext/matrix\_transform.hpp:
### transformPos
```lua
-- Transforms the given point by the the matrix, i.e.,
-- M * (p.x, p.y, p.z, 1). Note that this function cannot have a projection
vec3 = transformPos(m --[[ matrix ]], p --[[ vec3 ]])
```

### transformPosPerspective
```lua
-- Transforms the given point by the matrix4x4 (with a perspective divide)
vec3 = transformPosPerspective(m --[[ mat4x4 ]], p --[[ vec3 ]])
```

### transformDir
```lua
-- Transforms the given direction by the matrix, i.e.,
-- M * (dir.x, dir.y, dir.z, 0)
vec3 = transformDir(m --[[ matrix ]], dir --[[ vec3 ]])
```

### lookRotationRH
```lua
-- Create a right-handed rotation matrix for a given forward and up-vector
mat3x3 = lookRotationRH(fwd --[[ vec3 ]], up --[[ vec3 ]])
```

### lookRotationLH
```lua
--  Create a left-handed rotation matrix for a given forward and up-vector
mat3x3 = lookRotationLH(fwd --[[ vec3 ]], up --[[ vec3 ]])
```

### lookRotation
```lua
-- Create a rotation matrix for a given forward and up-vector, see
-- GLM_FORCE_LEFT_HANDED
mat3x3 = lookRotation(fwd --[[ vec3 ]], up --[[ vec3 ]])
```

### billboardRH
```lua
-- Creates a right-handed spherical billboard that rotates around a specified
-- object position
mat3x3 = billboardRH(
    objectPos --[[ vec3 ]],
    camPos --[[ vec3 ]],
    camUp --[[ vec3 ]],
    camFwd --[[ vec3 ]]
)
```

### billboardLH
```lua
-- Creates a left-handed spherical billboard that rotates around a specified
-- object position
mat3x3 = billboardLH(
    objectPos --[[ vec3 ]],
    camPos --[[ vec3 ]],
    camUp --[[ vec3 ]],
    camFwd --[[ vec3 ]]
)
```

### billboard
```lua
-- Creates a spherical billboard that rotates around a specified object
-- position, see GLM_FORCE_LEFT_HANDED
mat3x3 = billboard(
    objectPos --[[ vec3 ]],
    camPos --[[ vec3 ]],
    camUp --[[ vec3 ]],
    camFwd --[[ vec3 ]]
)
```

## glm/ext/quaternion\_common.hpp:
### invertible
```lua
-- Return true if the quaternion is invertible (i.e., is non-zero and finite)
bool = invertible(q --[[ quat ]])
```

### barycentric
```lua
-- Create a quaternion in barycentric coordinates
quat = barycentric(
    a --[[ quat ]],
    b --[[ quat ]],
    c --[[ quat ]],
    u --[[ number ]],
    v --[[ number ]]
)
```

## glm/ext/scalar\_common.hpp:
### loopRepeat
```lua
-- Return the shortest difference between two angles (radians)
angle = deltaAngle(a --[[ number ]], b --[[ number ]])
```

### loopRepeat
```lua
-- Loops "t" so that it is never greater than "length" and less than zero
vecN = loopRepeat(t --[[ vecN ]], length --[[ number ]])
vecN = loopRepeat(t --[[ vecN ]], length --[[ vecN ]])
```

### pingPong
```lua
-- Return a value that will increment and decrement between the value 0 and
-- length
vecN = pingPong(v --[[ vecN ]], length --[[ vecN ]])
```

### lerpAngle
```lua
-- A lerp implementation that ensures values interpolate correctly when they
-- wrap around two-pi (radians)
vecN = lerpAngle(from --[[ vecN ]], to --[[ vecN ]], t --[[ number ]])
vecN = lerpAngle(from --[[ vecN ]], to --[[ vecN ]], t --[[ vecN ]])
```

### slerp
```lua
-- Returns a spherical interpolation between two vectors
vecN = slerp(from --[[ vecN ]], to --[[ vecN ]], t --[[ number ]])
vecN = slerp(from --[[ number ]], to --[[ number ]], t --[[ number ]])

quat = slerp(from --[[ quat ]], to --[[ quat ]], t --[[ number ]])
```

## glm/ext/transform.hpp:
### trs
```lua
-- Creates a translation, rotation and scaling matrix
mat4x4 = trs(
    translation --[[ vec3 ]],
    rotation --[[ quat ]],
    scale --[[ vec3 ]]
)
```

## glm/gtc/epsilon.hpp:
### Aliases:
- `approximately = epsilonEqual`

## glm/gtc/quaternion.hpp:
### quatbillboardRH
```lua
-- See billboardRH
quat = quatbillboardRH(
    objectPos --[[ vec3 ]],
    camPos --[[ vec3 ]],
    camUp --[[ vec3 ]],
    camFwd --[[ vec3 ]]
)
```

### quatbillboardLH
```lua
-- See billboardLH
quat = quatbillboardLH(
    objectPos --[[ vec3 ]],
    camPos --[[ vec3 ]],
    camUp --[[ vec3 ]],
    camFwd --[[ vec3 ]]
)
```

### quatbillboard
```lua
-- See billboard
quat = quatbillboard(
    objectPos --[[ vec3 ]],
    camPos --[[ vec3 ]],
    camUp --[[ vec3 ]],
    camFwd --[[ vec3 ]]
)
```

### Aliases:
- `quatlookRotation = glm_quatLookAtLH`
- `quatlookRotationRH = glm_quatLookAtLH`
- `quatlookRotationLH = glm_quatLookAtLH`

## glm/gtx/matrix\_query.hpp:
### extractScale
```lua
-- Return the scaling components of the matrix
vecN = extractScale(m --[[ matNxM ]])
```

### hasUniformScale
```lua
-- Returns true if the matrix contains only uniform scaling (up to a given eps)
bool = hasUniformScale(m --[[ matNxM ]], epsilon --[[ number ]])
```

## glm/gtx/norm.hpp:
### Aliases:
- `sqrMagnitude = length2`
- `lengthSquared = length2`
- `distanceSquared = distance2`

## glm/gtx/orthonormalize_hpp
### orthonormalize3
```lua
-- Make the vectors normalized and orthogonal to one another
normal,tangent = orthonormalize3(normal --[[ vec3 ]], tangent --[[ vec3 ]])
normal,tangent,binormal = orthonormalize3(normal --[[ vec3 ]], tangent --[[ vec3 ]], binormal --[[ vec3 ]])
```

## glm/gtx/projection.hpp:
### projNorm
```lua
-- Project this vector 'v' onto a normalized direction vector
vproj --[[ vecN ]] = projNorm(v --[[ vecN ]], normal --[[ vecN ]])
```

### projPlane
```lua
-- Project a vector onto this plane defined by its orthogonal normal
vproj --[[ vecN ]] = projPlane(v --[[ vecN ]], normal --[[ vecN ]])
```

### projDecompose
```lua
-- Decompose the vector into parallel and perpendicular components with respect to a given direction
para --[[ vecN ]], perp --[[ vecN ]] = projDecompose(v --[[ vecN ]], dir --[[ vecN ]])
```

## glm/gtx/perpendicular.hpp:
### isPerpendicular
```lua
-- Return true if two vectors are perpendicular to one other
bool = isPerpendicular(v1 --[[ vecN ]], v2 --[[ vecN ]], epsilon --[[ number ]])
```

### perpendicular
```lua
-- Return a normalized direction vector that is perpendicular to "v" and the
-- specified hint vectors. If "v" points towards first hint vector, then the
-- second is used as a fallback
vec3 = perpendicular(v --[[ vec3 ]], hint --[[ vec3 ]], hint2 --[[ vec3 ]])
vec3 = perpendicular(v --[[ vec3 ]])
```

### perpendicular2
```lua
-- Return a normalized direction vector that is perpendicular to "v" and the
-- vector returned by glm::perpendicular
vec3 = perpendicular2(v --[[ vec3 ]], hint --[[ vec3 ]], hint2 --[[ vec3 ]])
vec3 = perpendicular2(v --[[ vec3 ]])
```

### perpendicularBasis
```lua
-- Returns two vectors that are orthogonal the vector and to each other
vec3,vec3 = perpendicularBasis(v --[[ vec3 ]])
```

### perpendicularFast
```lua
-- (Fast) Compute an orthonormal of the vector, i.e., compute a vector that is
-- mutually perpendicular to this axis
vec3 = perpendicularFast(v --[[ vec3 ]])
```

### Aliases:
- `basis = perpendicularBasis`

## glm/gtx/quaternion\_transform.hpp:
### rotateFromTo
```lua
-- Create a shortest arc quaternion that rotates a source direction to coincide
-- with the target
quat = rotateFromTo(source --[[ vec3 ]], target --[[ vec3 ]])
```

### rotateTowards
```lua
-- Return a rotation between two quaternions; rotating no further than
-- maxRadians
quat = rotateTowards(
    source --[[ quat ]],
    target --[[ quat ]],
    maxRadians --[[ number ]]
)
```

### Aliases:
- `transformDir = operator*(quat, vec3)`

## glm/gtx/rotate\_vector.hpp:
### barycentric
```lua
-- Return a vector containing the Cartesian coordinates of a point specified
-- in barycentric coordinates (i.e., relative to a N-dimensional triangle)
vecN = barycentric(
    a --[[ vecN ]],
    b --[[ vecN ]],
    c --[[ vecN ]],
    u --[[ number ]],
    v --[[ number ]]
)
```

## glm/gtx/vector\_angle.hpp:
### Aliases:
- `signedAngle = orientedAngle`

## glm/gtx/vector\_query.hpp:
### Aliases:
- `isZero = isNull`

## glm/gtx/euler\_angles.hpp:
### quatEulerAngleX
```lua
-- A quaternion from an euler angle X
quat = quatEulerAngleX(X --[[ number ]])
```

### quatEulerAngleY
```lua
-- A quaternion from an euler angle Y
quat = quatEulerAngleY(Y --[[ number ]])
```

### quatEulerAngleZ
```lua
-- A quaternion from an euler angle Z
quat = quatEulerAngleZ(Z --[[ number ]])
```

### quatEulerAngleXY
```lua
-- A quaternion from euler angles (X*Y)
quat = quatEulerAngleXY(X --[[ number ]], Y --[[ number ]])
```

### quatEulerAngleXZ
```lua
-- A quaternion from euler angles (X*Z)
quat = quatEulerAngleXZ(X --[[ number ]], Z --[[ number ]])
```

### quatEulerAngleYX
```lua
-- A quaternion from euler angles (Y*X)
quat = quatEulerAngleYX(Y --[[ number ]], X --[[ number ]])
```

### quatEulerAngleYZ
```lua
-- A quaternion from euler angles (Y*Z)
quat = quatEulerAngleYZ(Y --[[ number ]], Z --[[ number ]])
```

### quatEulerAngleZX
```lua
-- A quaternion from euler angles (Z*X)
quat = quatEulerAngleZX(Z --[[ number ]], X --[[ number ]])
```

### quatEulerAngleZY
```lua
-- A quaternion from euler angles (Z*Y)
quat = quatEulerAngleZY(Z --[[ number ]], Y --[[ number ]])
```

### quatEulerAngleXYX
```lua
-- A quaternion from euler angles (X*Y*X)
quat = quatEulerAngleXYX(X --[[ number ]], Y --[[ number ]], X --[[ number ]])
```

### quatEulerAngleXYZ
```lua
-- A quaternion from euler angles (X*Y*Z)
quat = quatEulerAngleXYZ(X --[[ number ]], Y --[[ number ]], Z --[[ number ]])
```

### quatEulerAngleXZX
```lua
-- A quaternion from euler angles (X*Z*X)
quat = quatEulerAngleXZX(X --[[ number ]], Z --[[ number ]], X --[[ number ]])
```

### quatEulerAngleXZY
```lua
-- A quaternion from euler angles (X*Z*Y)
quat = quatEulerAngleXZY(X --[[ number ]], Z --[[ number ]], Y --[[ number ]])
```

### quatEulerAngleYXY
```lua
-- A quaternion from euler angles (Y*X*Y)
quat = quatEulerAngleYXY(Y --[[ number ]], X --[[ number ]], Y --[[ number ]])
```

### quatEulerAngleYXZ
```lua
-- A quaternion from euler angles (Y*X*Z)
quat = quatEulerAngleYXZ(Y --[[ number ]], X --[[ number ]], Z --[[ number ]])
```

### quatEulerAngleYZX
```lua
-- A quaternion from euler angles (Y*Z*X)
quat = quatEulerAngleYZX(Y --[[ number ]], Z --[[ number ]], X --[[ number ]])
```

### quatEulerAngleYZY
```lua
-- A quaternion from euler angles (Y*Z*Y)
quat = quatEulerAngleYZY(Y --[[ number ]], Z --[[ number ]], Y --[[ number ]])
```

### quatEulerAngleZXY
```lua
-- A quaternion from euler angles (Z*X*Y)
quat = quatEulerAngleZXY(Z --[[ number ]], X --[[ number ]], Y --[[ number ]])
```

### quatEulerAngleZXZ
```lua
-- A quaternion from euler angles (Z*X*Z)
quat = quatEulerAngleZXZ(Z --[[ number ]], X --[[ number ]], Z --[[ number ]])
```

### quatEulerAngleZYX
```lua
-- A quaternion from euler angles (Z*Y*X)
quat = quatEulerAngleZYX(Z --[[ number ]], Y --[[ number ]], X --[[ number ]])
```

### quatEulerAngleZYZ
```lua
-- A quaternion from euler angles (Z*Y*Z)
quat = quatEulerAngleZYZ(Z --[[ number ]], Y --[[ number ]], Z --[[ number ]])
```

### Aliases:
- `eulerX = eulerAngleX`
- `eulerY = eulerAngleY`
- `eulerZ = eulerAngleZ`
- `eulerXZ = eulerAngleXZ`
- `eulerXY = eulerAngleXY`
- `eulerYX = eulerAngleYX`
- `eulerYZ = eulerAngleYZ`
- `eulerZX = eulerAngleZX`
- `eulerZY = eulerAngleZY`
- `eulerXYX = eulerAngleXYX`
- `eulerXYZ = eulerAngleXYZ`
- `eulerXZX = eulerAngleXZX`
- `eulerXZY = eulerAngleXZY`
- `eulerYXY = eulerAngleYXY`
- `eulerYXZ = eulerAngleYXZ`
- `eulerYZX = eulerAngleYZX`
- `eulerYZY = eulerAngleYZY`
- `eulerZXY = eulerAngleZXY`
- `eulerZXZ = eulerAngleZXZ`
- `eulerZYX = eulerAngleZYX`
- `eulerZYZ = eulerAngleZYZ`

# Preprocessor Header Definitions:
* COMMON_HPP
* EXPONENTIAL_HPP
* EXT_MATRIX_CLIP_SPACE_HPP
* EXT_MATRIX_COMMON_HPP
* EXT_MATRIX_PROJECTION_HPP
* EXT_MATRIX_RELATIONAL_HPP
* EXT_MATRIX_TRANSFORM_HPP
* EXT_QUATERNION_COMMON_HPP
* EXT_QUATERNION_EXPONENTIAL_HPP
* EXT_QUATERNION_GEOMETRIC_HPP
* EXT_QUATERNION_RELATIONAL_HPP
* EXT_QUATERNION_TRIGONOMETRIC_HPP
* EXT_SCALAR_COMMON_HPP
* EXT_SCALAR_INTEGER_HPP
* EXT_SCALAR_RELATIONAL_HPP
* EXT_SCALAR_ULP_HPP
* EXT_VECTOR_COMMON_HPP
* EXT_VECTOR_INTEGER_HPP
* EXT_VECTOR_RELATIONAL_HPP
* EXT_VECTOR_ULP_HPP
* GEOMETRIC_HPP
* GTC_BITFIELD_HPP
* GTC_COLOR_SPACE_HPP
* GTC_EPSILON_HPP
* GTC_INTEGER_HPP
* GTC_MATRIX_ACCESS_HPP
* GTC_MATRIX_INVERSE_HPP
* GTC_NOISE_HPP
* GTC_QUATERNION_HPP
* GTC_RANDOM_HPP
* GTC_RECIPROCAL_HPP
* GTC_ROUND_HPP
* GTC_TYPE_PRECISION_HPP
* GTC_ULP_HPP
* GTX_BIT_HPP
* GTX_CLOSEST_POINT_HPP
* GTX_COLOR_ENCODING_HPP
* GTX_COLOR_SPACE_HPP
* GTX_COLOR_SPACE_YCOCG_HPP
* GTX_COMMON_HPP
* GTX_COMPATIBILITY_HPP
* GTX_COMPONENT_WISE_HPP
* GTX_EASING_HPP
* GTX_EULER_ANGLES_HPP
* GTX_EXTEND_HPP
* GTX_EXTERIOR_PRODUCT_HPP
* GTX_FAST_EXPONENTIAL_HPP
* GTX_FAST_SQUARE_ROOT_HPP
* GTX_FAST_TRIGONOMETRY_HPP
* GTX_FUNCTIONS_HPP
* GTX_GRADIENT_PAINT_HPP
* GTX_HANDED_COORDINATE_SPACE_HPP
* GTX_INTEGER_HPP
* GTX_INTERSECT_HPP
* GTX_LOG_BASE_HPP
* GTX_LOG_BASE_HPP
* GTX_MATRIX_CROSS_PRODUCT_HPP
* GTX_MATRIX_DECOMPOSE_HPP
* GTX_MATRIX_FACTORISATION_HPP
* GTX_MATRIX_INTERPOLATION_HPP
* GTX_MATRIX_MAJOR_STORAGE_HPP
* GTX_MATRIX_OPERATION_HPP
* GTX_MATRIX_QUERY_HPP
* GTX_MATRIX_TRANSFORM_2D_HPP
* GTX_MIXED_PRODUCT_HPP
* GTX_NORMALIZE_DOT_HPP
* GTX_NORMAL_HPP
* GTX_NORM_HPP
* GTX_OPTIMUM_POW_HPP
* GTX_ORTHONORMALIZE_HPP
* GTX_PERPENDICULAR_HPP
* GTX_POLAR_COORDINATES_HPP
* GTX_PROJECTION_HPP
* GTX_QUATERNION_HPP
* GTX_QUATERNION_TRANSFORM_HPP
* GTX_RANGE_HPP
* GTX_ROTATE_NORMALIZED_AXIS_HPP
* GTX_ROTATE_VECTOR_HPP
* GTX_SPLINE_HPP
* GTX_TEXTURE_HPP
* GTX_TRANSFORM2_HPP
* GTX_TRANSFORM_HPP
* GTX_VECTOR_ANGLE_HPP
* GTX_VECTOR_QUERY_HPP
* GTX_WRAP_HPP
* INTEGER_HPP
* MATRIX_HPP
* PACKING_HPP
* TRIGONOMETRIC_HPP
* VECTOR_RELATIONAL_HPP
* CONSTANTS_HPP
* EXT_SCALAR_CONSTANTS_HPP
