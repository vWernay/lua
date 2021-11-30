--[[
================================================================================
https://www.kevinbeason.com/smallpt/
================================================================================
This script is used for testing, profiling, PGO'ing, etc. Does not cover the
entirety of the VM, binding library, or any matrix/quat bits.

@LICENSE
    See Copyright Notice in lua.h
--]]
local glm = require('glm')
local vec3 = vec3

local abs = math.abs
local cos = math.cos
local max = math.max
local rand = math.random
local sin = math.sin
local sqrt = math.sqrt
local pi <const> = math.pi
local huge <const> = math.huge

local clamp = glm.clamp
local compMax = glm.compMax
local cross = glm.cross
local dot = glm.dot
local feps = glm.feps
local norm = glm.norm
local intersectsSphere = glm.ray.intersectsSphere
local reflect = glm.reflect
local refract = glm.refract

local SCENES = { }
local SHADERS = { }

---------------------------------------
---------------- Scene ----------------
---------------------------------------

--[[ Create a new scene primitive --]]
local function Sphere(radius, position, emission, color, reflType)
    return {
        position = position,
        radius = radius,
        emission = emission,
        color = color,
        reflectionType = reflType,
    }
end

--[[ Find the the closest primitive that intersects the ray --]]
local function SceneIntersection(scene, rayOrig, rayDir)
    local t = huge
    local hitPrim = nil
    for i = 1,#scene do
        local sphere = scene[i]
        local d = nil

        -- The current GLM Binding implementation does not allow configurable
        -- epsilon values for the intersection tests. Therefore apply a less
        -- sensitive epsilon after-the-fact.
        local count,near,far = intersectsSphere(rayOrig, rayDir, sphere.position, sphere.radius)
        if count > 0 and near > 5e-2 then
            d = near
        elseif count > 1 and far > 5e-2 then
            d = far
        end

        if d ~= nil and d < t then
            t,hitPrim = d,sphere
        end
    end
    return t,hitPrim
end

do
    SCENES['cornellbox'] = {
        Sphere(1e5, vec3( 1e5+1,40.8,81.6), vec3(),vec3(.75,.25,.25),'DIFF'), -- Left
        Sphere(1e5, vec3(-1e5+99,40.8,81.6),vec3(),vec3(.25,.25,.75),'DIFF'), -- Right
        Sphere(1e5, vec3(50,40.8, 1e5), vec3(),vec3(.75,.75,.75),'DIFF'), -- Back
        Sphere(1e5, vec3(50,40.8,-1e5+170), vec3(),vec3(), 'DIFF'), -- Front
        Sphere(1e5, vec3(50, 1e5, 81.6), vec3(),vec3(.75,.75,.75),'DIFF'), -- Bottom
        Sphere(1e5, vec3(50,-1e5+81.6,81.6),vec3(),vec3(.75,.75,.75),'DIFF'), -- Top
        Sphere(16.5, vec3(27,16.5,47), vec3(),vec3(1,1,1)*.999, 'SPEC'), -- Mirror
        Sphere(16.5, vec3(73,16.5,78), vec3(),vec3(1,1,1)*.999, 'REFR'), -- Glass
        Sphere(600, vec3(50,681.6-.27,81.6),vec3(12,12,12), vec3(), 'DIFF'), -- Lite
    }
end

do
    local FTC <const> = vec3(0.0588, 0.361, 0.0941)
    local FSC <const> = vec3(0.7)
    SCENES['forest'] = {
        Sphere(1e5, vec3(50,1e5+130,0), vec3(1.3), vec3(), 'DIFF'), -- Light
        Sphere(1e2, vec3(50,-1e2+2,47), vec3(), vec3(0.7), 'DIFF'), -- Ground
        Sphere(1e4, vec3(50,-30,300)+vec3(-sin(50*pi/180),0,cos(50*pi/180))*1e4, vec3(), vec3(0.99), 'SPEC'), -- Mirror L
        Sphere(1e4, vec3(50,-30,300)+vec3(sin(50*pi/180),0,cos(50*pi/180))*1e4, vec3(), vec3(0.99), 'SPEC'), -- Mirror R
        Sphere(1e4, vec3(50,-30,-50)+vec3(-sin(30*pi/180),0,-cos(30*pi/180))*1e4, vec3(), vec3(0.99), 'SPEC'), -- Mirror FL
        Sphere(1e4, vec3(50,-30,-50)+vec3(sin(30*pi/180),0,-cos(30*pi/180))*1e4, vec3(), vec3(0.99), 'SPEC'), -- Mirror
        Sphere(4, vec3(50,6*.6,47), vec3(), vec3(.13,.066,.033), 'DIFF'), -- Tree
        Sphere(16, vec3(50,6*2+16*.6,47), vec3(), FTC, 'DIFF'), -- Tree
        Sphere(11, vec3(50,6*2+16*.6*2+11*.6,47), vec3(), FTC, 'DIFF'), -- Tree
        Sphere(7, vec3(50,6*2+16*.6*2+11*.6*2+7*.6,47), vec3(),FTC, 'DIFF'), -- Tree
        Sphere(15.5, vec3(50,1.8+6*2+16*.6,47), vec3(), FSC, 'DIFF'), -- Tree
        Sphere(10.5, vec3(50,1.8+6*2+16*.6*2+11*.6,47), vec3(), FSC, 'DIFF'), -- Tree
        Sphere(6.5, vec3(50,1.8+6*2+16*.6*2+11*.6*2+7*.6,47), vec3(), FSC, 'DIFF'), -- Tree
    }
end

do
    local IC <const> = vec3(50,-20,-860)
    SCENES['island'] = {
        Sphere(160, IC+vec3(0,600,-500), vec3(1,1,1)*2e2, vec3(), 'DIFF'), -- Sun
        Sphere(800, IC+vec3(0,-880,-9120), vec3(1,1,1)*2e1, vec3(), 'DIFF'), -- Horizon
        Sphere(10000, IC+vec3(0,0,-200), vec3(0.0627, 0.188, 0.569), vec3(0.4), 'DIFF'), -- Sky
        Sphere(800, IC+vec3(0,-720,-200), vec3(), vec3(0.110, 0.898, 1.00)*.996, 'REFR'), -- Water
        Sphere(790, IC+vec3(0,-720,-200),vec3(), vec3(.4,.3,.04)*.6, 'DIFF'), -- Earth
        Sphere(325, IC+vec3(0,-255,-50), vec3(), vec3(.4,.3,.04)*.8, 'DIFF'), -- Island
        Sphere(275, IC+vec3(0,-205,-33), vec3(), vec3(.02,.3,.02)*.75, 'DIFF'), -- Grass
    }
end

do
    SCENES['nightsky'] = {
        Sphere(2.5e3, vec3(.82,.92,-2)*1e4, vec3(0.8e2), vec3(), 'DIFF'), -- Moon
        Sphere(2.5e4, vec3(50,0.0,0.0), vec3(0.114, 0.133, 0.212)*1e-2, vec3(.216,.384,1)*0.003, 'DIFF'), -- Sky
        Sphere(5e0, vec3(-.2,0.16,-1)*1e4, vec3(1.00, 0.843, 0.698)*1e2, vec3(), 'DIFF'), -- Star
        Sphere(5e0, vec3(0.0,0.18,-1)*1e4, vec3(1.00, 0.851, 0.710)*1e2, vec3(), 'DIFF'), -- Star2
        Sphere(5e0, vec3(0.3, 0.15,-1)*1e4, vec3(0.671, 0.780, 1.00)*1e2, vec3(), 'DIFF'), -- Star3
        Sphere(3.5e4, vec3(600,-3.5e4+1,300), vec3(), vec3(.6,.8,1)*.01, 'REFR'), -- Pool
        Sphere(5e4, vec3(-500,-5e4+0,0.0), vec3(), vec3(0.35), 'DIFF'), -- Hill
        Sphere(6.5, vec3(27,0.0,47), vec3(), vec3(0.33), 'DIFF'), -- Hut
        Sphere(7, vec3(27+8*sqrt(2),0.0,47+8*sqrt(2)), vec3(), vec3(0.33), 'DIFF'), -- Door
        Sphere(500, vec3(-1e3,-300,-3e3), vec3(), vec3(0.351), 'DIFF'), -- Mount
        Sphere(830, vec3(0.0,-500,-3e3), vec3(), vec3(0.354), 'DIFF'), -- Mount2
        Sphere(490, vec3(1e3,-300,-3e3), vec3(), vec3(0.352), 'DIFF'), -- Mount3
    }
end

do
    local SkyC <const> = vec3(50, 40.8, -860)
    local SkyEmission <const> = vec3(0.00063842, 0.02001478, 0.28923243)*6e-2*8
    SCENES['sky'] = {
        Sphere(1600, vec3(1,0,2)*3000, vec3(1,.9,.8)*1.2e1*1.56*2, vec3(), 'DIFF'), -- Sun
        Sphere(1560, vec3(1,0,2)*3500, vec3(1,.5,.05)*4.8e1*1.56*2, vec3(), 'DIFF'), -- Horizon sun2
        Sphere(10000, SkyC+vec3(0,0,-200), SkyEmission, vec3(.7,.7,1)*.25, 'DIFF'), -- Sky
        Sphere(100000, vec3(50,-100000,0), vec3(), vec3(0.3), 'DIFF'), -- Ground
        Sphere(110000, vec3(50,-110048.5,0), vec3(.9,.5,.05)*4, vec3(), 'DIFF'), -- Horizon brightener
        Sphere(4e4, vec3(50,-4e4-30,-3000), vec3(), vec3(0.2), 'DIFF'), -- Mountains
        Sphere(26.5, vec3(22,26.5,42), vec3(), vec3(0.596), 'SPEC'), -- White Mirror
        Sphere(13, vec3(75,13,82), vec3(), vec3(.96,.96,.96)*.96, 'REFR'), -- Glass
        Sphere(22, vec3(87,22,24), vec3(), vec3(.6,.6,.6)*.696, 'REFR') -- Glass2
    }
end

do
    local VC <const> = vec3(50,-20,-860)
    SCENES['vista'] = {
        Sphere(8000, VC+vec3(0,-8000,-900), vec3(1,.4,.1)*5e-1, vec3(), 'DIFF'), -- Sun
        Sphere(1e4, VC+vec3(), vec3(0.631, 0.753, 1.00)*3e-1, vec3(1)*.5, 'DIFF'), -- Sky
        Sphere(150, VC+vec3(-350,0, -100), vec3(), vec3(1)*.3, 'DIFF'), -- Mountain
        Sphere(200, VC+vec3(-210,0,-100), vec3(), vec3(1)*.3, 'DIFF'), -- Mountain2
        Sphere(145, VC+vec3(-210,85,-100), vec3(), vec3(1)*.8, 'DIFF'), -- Snow
        Sphere(150, VC+vec3(-50,0,-100), vec3(), vec3(1)*.3, 'DIFF'), -- Mountain3
        Sphere(150, VC+vec3(100,0,-100), vec3(), vec3(1)*.3, 'DIFF'), -- Mountain4
        Sphere(125, VC+vec3(250,0,-100), vec3(), vec3(1)*.3, 'DIFF'), -- Mountain5
        Sphere(150, VC+vec3(375,0,-100), vec3(), vec3(1)*.3, 'DIFF'), -- Mountain6
        Sphere(2500, VC+vec3(0,-2400,-500), vec3(), vec3(1)*.1, 'DIFF'), -- Base
        Sphere(8000, VC+vec3(0,-8000,200), vec3(), vec3(.2,.2,1), 'REFR'), -- Mountain7
        Sphere(8000, VC+vec3(0,-8000,1100), vec3(), vec3(0,.3,0), 'DIFF'), -- Grass
        Sphere(8, VC+vec3(-75, -5, 850), vec3(), vec3(0,.3,0), 'DIFF'), -- Bush
        Sphere(30, VC+vec3(0, 23, 825), vec3(), vec3(1)*.996, 'REFR'), -- Ball
        Sphere(30, VC+vec3(200,280,-400), vec3(), vec3(1)*.8, 'DIFF'), -- Clouds
        Sphere(37, VC+vec3(237,280,-400), vec3(), vec3(1)*.8, 'DIFF'), -- Clouds2
        Sphere(28, VC+vec3(267,280,-400), vec3(), vec3(1)*.8, 'DIFF'), -- clouds3
        Sphere(40, VC+vec3(150,280,-1000), vec3(), vec3(1)*.8, 'DIFF'), -- clouds4
        Sphere(37, VC+vec3(187,280,-1000), vec3(), vec3(1)*.8, 'DIFF'), -- clouds5
        Sphere(40, VC+vec3(600,280,-1100), vec3(), vec3(1)*.8, 'DIFF'), -- clouds6
        Sphere(37, VC+vec3(637,280,-1100), vec3(), vec3(1)*.8, 'DIFF'), -- clouds7
        Sphere(37, VC+vec3(-800,280,-1400), vec3(), vec3(1)*.8, 'DIFF'), -- clouds8
        Sphere(37, VC+vec3(0,280,-1600), vec3(), vec3(1)*.8, 'DIFF'), -- clouds9
        Sphere(37, VC+vec3(537,280,-1800), vec3(), vec3(1)*.8, 'DIFF'), -- clouds10
    }
end

do
    local D <const> = 50
    local R <const> = 40
    SCENES['overlap'] = {
        Sphere(150, vec3(50+75,28,62), vec3(1,1,1)*0e-3, vec3(1,.9,.8)*.93, 'REFR'),
        Sphere(28, vec3(50+5,-28,62), vec3(1,1,1)*1e1, vec3(1,1,1)*0, 'DIFF'),
        Sphere(300, vec3(50,28,62), vec3(1,1,1)*0e-3, vec3(1,1,1)*.93, 'SPEC')
    }
end

do
    local R <const> = 60
    local T <const> = 30 * pi/180
    local D <const> = R / cos(T)
    local Z <const> = 60
    SCENES['wada'] = {
        Sphere(1e5, vec3(50, 100, 0), vec3(1,1,1)*3e0, vec3(), 'DIFF'), -- sky
        Sphere(1e5, vec3(50, -1e5-D-R, 0), vec3(), vec3(.1,.1,.1), 'DIFF'), -- grnd
        Sphere(R, vec3(50,40.8,62)+vec3( cos(T),sin(T),0)*D, vec3(), vec3(1,.3,.3)*.999, 'SPEC'), -- red
        Sphere(R, vec3(50,40.8,62)+vec3(-cos(T),sin(T),0)*D, vec3(), vec3(.3,1,.3)*.999, 'SPEC'), -- grn
        Sphere(R, vec3(50,40.8,62)+vec3(0,-1,0)*D, vec3(), vec3(.3,.3,1)*.999, 'SPEC'), -- blue
        Sphere(R, vec3(50,40.8,62)+vec3(0,0,-1)*D, vec3(), vec3(.53,.53,.53)*.999, 'SPEC'), -- back
        Sphere(R, vec3(50,40.8,62)+vec3(0,0,1)*D, vec3(), vec3(1,1,1)*.999, 'REFR'), -- front
    }
end

do
    local R <const> = 120
    local T <const> = 30 * pi/180
    local D <const> = R / cos(T)
    local Z <const> = 62
    local C <const> = vec3(0.275, 0.612, 0.949)
    local FR <const> = 2*2*R*2*sqrt(2./3.0)-R*2*sqrt(2./3.0)/3.0
    SCENES['wada2'] = {
        Sphere(R, vec3(50,28,Z)+vec3( cos(T),sin(T),0)*D, C*6e-2, vec3(0.996), 'SPEC'), -- Red
        Sphere(R, vec3(50,28,Z)+vec3(-cos(T),sin(T),0)*D, C*6e-2, vec3(0.996), 'SPEC'), -- Green
        Sphere(R, vec3(50,28,Z)+vec3(0,-1,0)*D, C*6e-2, vec3(0.996), 'SPEC'), -- Blue
        Sphere(R, vec3(50,28,Z)+vec3(0,0,-1)*R*2*sqrt(2./3.0), C*0e-2, vec3(0.996), 'SPEC'), -- Back
        Sphere(FR, vec3(50,28,Z)+vec3(0,0,-R*2*sqrt(2./3.0)/3.0), vec3(1,1,1)*0, vec3(0.5), 'SPEC'), -- Front
    }
end

---------------------------------------
--------------- Shaders ---------------
---------------------------------------

-- Return an estimated radiance along a given ray
local function Radiance(scene, rayOrig, rayDir, depth, maxDepth)
    local hitDistance,hitPrim = SceneIntersection(scene, rayOrig, rayDir)
    if hitPrim then
        local hitPoint = rayOrig + (rayDir * hitDistance)
        local hitNormal = norm(hitPoint - hitPrim.position)
        local hitColor = hitPrim.color

        depth = depth + 1
        local p = compMax(hitColor)
        if depth > maxDepth then -- Russian roulette beyond max depth
            if rand() < p then
                hitColor = hitColor * (1 / p)
            else
                return hitPrim.emission
            end
        end

        return SHADERS[hitPrim.reflectionType](
            scene,
            rayOrig, rayDir,
            hitPoint, hitNormal, hitColor,
            hitPrim.emission,
            depth, maxDepth
        )
    end
    return vec3()
end

-- Ideal SPECULAR reflection
SHADERS['SPEC'] = function(scene, rayOrig, rayDir, hitPoint, hitNormal, hitColor, emColor, depth, maxDepth)
    local rayDir = reflect(rayDir, hitNormal)
    return emColor + hitColor * Radiance(scene, hitPoint, rayDir, depth, maxDepth)
end

-- Ideal DIFFUSE reflection
SHADERS['DIFF'] = function(scene, rayOrig, rayDir, hitPoint, hitNormal, hitColor, emColor, depth, maxDepth)
    local r1 = 2 * pi * rand()
    local r2 = rand()
    local r2s = sqrt(r2)

    local w = (dot(hitNormal, rayDir) > 0 and -hitNormal) or hitNormal
    local u = norm(cross((abs(w.x) > 0.1 and vec3(0,1,0) or vec3(1,0,0)), w))
    local v = cross(w, u)

    local d = norm(u * (cos(r1) * r2s) + v * (sin(r1) * r2s) + w * (sqrt(1 - r2)))
    return emColor + hitColor * Radiance(scene, hitPoint, d, depth, maxDepth)
end

--[[
    Ideal dielectric REFRACTION

    @TODO: Verify/Improve
--]]
SHADERS['REFR'] = function(scene, rayOrig, rayDir, hitPoint, hitNormal, hitColor, emColor, depth, maxDepth)
    local NC <const> = 1.0
    local NT <const> = 1.5

    local d = dot(hitNormal, rayDir)
    local into = d < 0 -- Ray from outside going in?
    local n = into and hitNormal or -hitNormal

    local relfDir = rayDir * (2 * d)
    local refrDir = refract(rayDir, n, (into and NC/NT) or NT/NC)

    local newRadiance = 0
    if #refrDir < feps then -- Total internal reflection
        newRadiance = Radiance(scene, hitPoint, relfDir, depth, maxDepth)
    else -- Russian Roulette
        local a = NT - NC
        local b = NT + NC
        local r0 = (a * a) / (b * b)
        local c = 1 - ((into and -dot(rayDir, n)) or dot(refrDir, hitNormal))
        local re = r0 + (1 - r0) * c * c * c * c * c
        local tr = 1 - re
        local p = 0.25 + 0.5 * re
        local rp = re / p
        local tp = tr / (1 - p)

        newRadiance = depth > 2 and rand() < p
            and (Radiance(scene, hitPoint, relfDir, depth, maxDepth) * rp)
            or (Radiance(scene, hitPoint, refrDir, depth, maxDepth) * tp)
            or (Radiance(scene, hitPoint, relfDir, depth, maxDepth) * re
                + Radiance(scene, hitPoint, refrDir, depth, maxDepth) * tr)
    end
    return emColor + (hitColor * newRadiance)
end

----------------------------------------
--------------- Renderer ---------------
----------------------------------------

-- Return a flat image array
local pixelArray = function(w, h)
    local m = {}
    for i=1,w*h do m[i]= 0 end
    return m
end

-- Applies Gamma correction/tone on a given value
local gammaCorrect = function(x, factor, range)
    return math.floor(math.pow(x, 1 / factor) * range + 0.5)
end

-- Remaps 'm' according to 'f'
local mapFunc = function(m, f, ...)
    for i=1,#m do
        m[i] = vec3(
            f(m[i].x, ...),
            f(m[i].y, ...),
            f(m[i].z, ...)
        )
    end
    return m
end

-- PrettyProgressPrint
local last_str = ''
local function PrintProgress(str)
   io.stderr:write(('\b \b'):rep(#last_str))
   io.stderr:write(str)
   io.stderr:flush()
   last_str = str
end

-- Rendering loop
local function Render(scene, w, h, aa, spp, maxDepth, camPosition, camDirection, camFov)
    local ColorRange <const> = 255
    local GammaFactor <const> = 2.2
    local InvSqAA <const> = 1.0 / (aa * aa)

    local c = pixelArray(w, h)
    local cx = vec3(w * cos(camFov) / h, 0, 0)
    local cy = norm(cross(cx, camDirection)) * cos(camFov)

    local start_time = os.clock()
    PrintProgress("")
    for y=1,h do
        PrintProgress(("%d%% Completed"):format(math.floor(y / h * 100)))
        for x=1,w do
            local i = x + ((h - 1) - (y - 1)) * w
            for sy=0,aa-1 do
                for sx=0,aa-1 do
                    local r = vec3()
                    for s=1,spp do
                        local r1 = 2 * rand()
                        local r2 = 2 * rand()

                        local dx = r1 < 1 and sqrt(r1) - 1 or 1 - sqrt(2 - r1)
                        local dy = r2 < 1 and sqrt(r2) - 1 or 1 - sqrt(2 - r2)
                        local dir = norm(
                            (cx * (((sx + 0.5 + dx) / 2 + x) / w - 0.5)) +
                            (cy * (((sy + 0.5 + dy) / 2 + y) / h - 0.5)) +
                            camDirection
                        )

                        local nr = Radiance(scene, camPosition + dir * 140, dir, 0, maxDepth)
                        r = r +  nr * (1 / spp)
                    end

                    c[i] = c[i] + (clamp(r) * InvSqAA)
                end
            end
        end
    end
    PrintProgress("")

    print("Tracing took: " .. (os.clock() - start_time) .. " seconds")
    return mapFunc(c, gammaCorrect, GammaFactor, ColorRange) -- Apply gamma correction
end

--[[ Render a scene as a PPM Image --]]
local function RenderPPM(scene, render, outputPath)
    assert(scene ~= nil)
    local m = Render(scene, -- Scene to be rendered
        render.width, render.height,
        render.antialiasing, render.samples, render.maxDepth,
        render.cameraPosition, render.cameraDirection, render.cameraFov
    )

    local file = assert(io.open(outputPath, "wb"))
    file:write(string.format("P3\n%d %d\n%d\n", render.width, render.height, 255))
    for i=1,#m do
        file:write(string.format("%d %d %d ",
            math.floor(m[i].x),
            math.floor(m[i].y),
            math.floor(m[i].z))
        )
    end
    file:close()
    print(("Written to: %s"):format(outputPath))
end

----------------------------------------
----------------- Main -----------------
----------------------------------------

local InputScene = arg[1] or 'cornellbox'
local OutputPath = arg[2] or ("%s.ppm"):format(InputScene)
local RenderParameters = {
    -- Viewport
    width = 1024/2,
    height = 768/2,
    -- Depth, Anti-Aliasing, Samples-Per-Pixel
    antialiasing = 2,
    maxDepth = 5,
    samples = 32,
    -- Camera Information
    cameraPosition = vec3(50, 52, 295.6),
    cameraDirection = norm(vec3(0, -0.042612, -1)),
    cameraFov = 1.03153,
}

-- math.randomseed(300)
if SCENES[InputScene] then
    RenderPPM(SCENES[InputScene], RenderParameters, OutputPath)
else
    local sorted = { }
    for k,_ in pairs(SCENES) do
        sorted[#sorted + 1] = k
    end

    table.sort(sorted)
    error(("Available Scenes: %s"):format(table.concat(sorted, ", ")))
end
