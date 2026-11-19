--
-- Terrain coloring based on genland pipeline (Tom Dobrowolski / Ken Silverman).
-- Slope-based grass tones, water tint, ambient, directional light, shadow rays,
-- shadow blur, and final merge - applied to existing voxel columns.
-- Adapted for vengi (Y-up): columns are along Y axis, XZ is the horizontal plane.
--

function arguments()
	return {
		{ name = 'seed', desc = 'noise seed', type = 'int', default = '0', min = '0' },
		{ name = 'grassslopeexp', desc = 'grass slope exponent (higher = more ground on slopes)', type = 'float', default = '1.65', min = '0.05', max = '8.0' },
		{ name = 'grassslopegain', desc = 'grass slope gain multiplier', type = 'float', default = '1.3', min = '0.0', max = '10.0' },
		{ name = 'grassheightscale', desc = 'height scale for grass falloff (voxels)', type = 'float', default = '32.0', min = '0.5', max = '200.0' },
		{ name = 'shadowstrength', desc = 'shadow darkness (0-255)', type = 'float', default = '33.0', min = '0.0', max = '255.0' },
		{ name = 'shadowblur', desc = 'shadow blur radius in cells (0=sharp)', type = 'int', default = '1', min = '0', max = '16' },
		{ name = 'sunheightstep', desc = 'shadow sun elevation (larger=shorter shadows)', type = 'float', default = '0.44', min = '0.01', max = '3.0' },
		{ name = 'ambience', desc = 'ambient light factor', type = 'float', default = '0.22', min = '0.0', max = '1.0' },
		{ name = 'directional', desc = 'directional light intensity', type = 'float', default = '1.2', min = '0.0', max = '4.0' },
		{ name = 'waterlayers', desc = 'bottom voxel rows using water color (0=off)', type = 'int', default = '1', min = '0', max = '64' }
	}
end

function description()
	return "Applies terrain coloring (grass/ground/water + lighting + shadows) to existing voxel columns based on genland algorithm."
end

local function clamp(v, lo, hi)
	if v < lo then return lo end
	if v > hi then return hi end
	return v
end

local function lerp(a, b, t)
	return a + (b - a) * t
end

-- Simple hash for per-cell variation (uncorrelated [-1,1])
local function cellHash(x, z, s)
	-- Lua 5.1 integers via bit ops emulation using math
	local u = (x * 1664525 + z * 1013904223 + s * 1103515245) % 4294967296
	u = math.floor(u / 65536) % 65536
	return u / 32768.0 - 1.0
end

function main(node, region, color, seed, grassslopeexp, grassslopegain, grassheightscale,
	shadowstrength, shadowblur, sunheightstep, ambience, directional, waterlayers)

	local volume = node:volume()
	local palette = node:palette()
	local mins = region:mins()
	local maxs = region:maxs()

	local gw = maxs.x - mins.x + 1
	local gd = maxs.z - mins.z + 1
	local dy = maxs.y - mins.y + 1

	-- Colors
	local groundR, groundG, groundB = 140, 125, 115
	local grass1R, grass1G, grass1B = 72, 80, 32
	local grass2R, grass2G, grass2B = 68, 78, 40
	local waterR, waterG, waterB = 50, 90, 110

	-- Pass 1: build height map (top Y per column)
	local hgt = {}
	for iz = 0, gd - 1 do
		for ix = 0, gw - 1 do
			local top = -1
			for iy = dy - 1, 0, -1 do
				if volume:voxel(mins.x + ix, mins.y + iy, mins.z + iz) ~= -1 then
					top = iy
					break
				end
			end
			hgt[iz * gw + ix] = top
		end
	end

	-- Helper to sample height with bounds clamping
	local function sampleH(ix, iz)
		ix = clamp(ix, 0, gw - 1)
		iz = clamp(iz, 0, gd - 1)
		return hgt[iz * gw + ix]
	end

	-- Pass 2: compute per-column color and lighting
	local bufR = {}
	local bufG = {}
	local bufB = {}
	local ambR = {}
	local ambG = {}
	local ambB = {}
	local sh = {}

	local halfSpan = 3

	for iz = 0, gd - 1 do
		for ix = 0, gw - 1 do
			local idx = iz * gw + ix
			local h0 = hgt[idx]
			if h0 < 0 then
				bufR[idx] = 0
				bufG[idx] = 0
				bufB[idx] = 0
				ambR[idx] = 0
				ambG[idx] = 0
				ambB[idx] = 0
				sh[idx] = 0
			else
				-- Slopes from height differences
				local denom = 2.0 * halfSpan
				local hx = (sampleH(ix + halfSpan, iz) - sampleH(ix - halfSpan, iz)) / denom
				local hz = (sampleH(ix, iz + halfSpan) - sampleH(ix, iz - halfSpan)) / denom

				-- Normal (pointing up is -Y in slope space)
				local nx = hx
				local nz = hz
				local ny = -1.0
				local invLen = 1.0 / math.sqrt(nx * nx + ny * ny + nz * nz)
				nx = nx * invLen
				ny = ny * invLen
				nz = nz * invLen

				-- Start with ground color
				local cr, cg, cb = groundR, groundG, groundB

				-- Grass blending based on slope
				local sUp = clamp(-ny, 0.0, 1.0)
				local slopeTerm = grassslopegain * (sUp ^ grassslopeexp)
				local macroN = g_noise.noise3(ix / 64.0, iz / 64.0, 0.3 + seed * 0.01) * 0.3
				local fineN = g_noise.noise3(ix * 0.41 + 2.17, iz * 0.41 - 1.03, 1.88 + seed * 0.01) * 0.38
				local cellN = cellHash(ix, iz, seed)
				local detailRaw = 0.2 * (0.55 * fineN + 0.4 * cellN)
				local rawGrass = slopeTerm - h0 / grassheightscale + macroN + detailRaw
				local grassBlend = clamp(rawGrass, 0.0, 1.0)

				cr = lerp(cr, grass1R, grassBlend)
				cg = lerp(cg, grass1G, grassBlend)
				cb = lerp(cb, grass1B, grassBlend)

				-- Secondary grass tone at mid-blend
				local secBlend = (1.0 - math.abs(grassBlend - 0.5) * 2.0) * 0.7
				cr = lerp(cr, grass2R, secBlend)
				cg = lerp(cg, grass2G, secBlend)
				cb = lerp(cb, grass2B, secBlend)

				-- Rugged color noise (multiplicative)
				local ruggedStr = 1.4 * grassBlend
				if ruggedStr > 0.001 then
					local pn = g_noise.noise3(ix * 0.74 + 0.41, iz * 0.74 - 0.19, 1.07 + seed * 0.01)
					local hn = 0.5 * cellHash(ix, iz, seed + 2011) + 0.5 * cellHash(ix + 97, iz - 43, seed + 2711)
					local n = clamp(0.64 * pn + 0.36 * hn, -1.0, 1.0)
					local swing = math.min(0.22 * ruggedStr, 0.58)
					local m = 1.0 + swing * n
					cr = clamp(cr * m, 0, 255)
					cg = clamp(cg * m, 0, 255)
					cb = clamp(cb * m, 0, 255)
				end

				-- Directional light (N dot L, fixed sun direction like genland)
				local sunLight = (nx * 0.5 + nz * 0.25 - ny) / math.sqrt(0.5 * 0.5 + 0.25 * 0.25 + 1.0)
				sunLight = sunLight * directional

				-- Ambient
				local ar = clamp(math.floor(cr * ambience), 0, 255)
				local ag = clamp(math.floor(cg * ambience), 0, 255)
				local ab = clamp(math.floor(cb * ambience), 0, 255)
				ambR[idx] = ar
				ambG[idx] = ag
				ambB[idx] = ab

				local maxAmb = math.max(ar, math.max(ag, ab))
				bufR[idx] = clamp(math.floor(cr * sunLight), 0, 255 - maxAmb)
				bufG[idx] = clamp(math.floor(cg * sunLight), 0, 255 - maxAmb)
				bufB[idx] = clamp(math.floor(cb * sunLight), 0, 255 - maxAmb)
				sh[idx] = 0
			end
		end
	end

	-- Pass 3: shadow casting
	local shadowRange = math.max(8, math.floor(math.max(gw, gd) / 4))
	for iz = 0, gd - 1 do
		for ix = 0, gw - 1 do
			local idx = iz * gw + ix
			if hgt[idx] >= 0 then
				local shadowCheck = hgt[idx] + sunheightstep
				for si = 1, shadowRange - 1 do
					local sz = ((iz - math.floor(si / 2)) % gd + gd) % gd
					local sx = ((ix - si) % gw + gw) % gw
					if hgt[sz * gw + sx] > shadowCheck then
						sh[idx] = clamp(math.floor(shadowstrength + 0.5), 0, 255)
						break
					end
					shadowCheck = shadowCheck + sunheightstep
				end
			end
		end
	end

	-- Pass 4: shadow blur
	if shadowblur > 0 then
		local shTmp = {}
		local side = 2 * shadowblur + 1
		local area = side * side
		for iz = 0, gd - 1 do
			for ix = 0, gw - 1 do
				local sum = 0
				for dz = -shadowblur, shadowblur do
					local zz = ((iz + dz) % gd + gd) % gd
					for dx = -shadowblur, shadowblur do
						local xx = ((ix + dx) % gw + gw) % gw
						sum = sum + sh[zz * gw + xx]
					end
				end
				shTmp[iz * gw + ix] = math.floor((sum + area / 2) / area)
			end
		end
		sh = shTmp
	end

	-- Pass 5: final merge - paint all voxels in each column
	for iz = 0, gd - 1 do
		for ix = 0, gw - 1 do
			local idx = iz * gw + ix
			local ytop = hgt[idx]
			if ytop >= 0 then
				local colorIdx = 256 - (sh[idx] * 4)
				local fr = clamp(math.floor(bufR[idx] * colorIdx / 256) + ambR[idx], 0, 255)
				local fg = clamp(math.floor(bufG[idx] * colorIdx / 256) + ambG[idx], 0, 255)
				local fb = clamp(math.floor(bufB[idx] * colorIdx / 256) + ambB[idx], 0, 255)

				-- Water color for bottom layers
				local wr, wg, wb
				if waterlayers > 0 then
					local wJit = g_noise.noise3(ix * 0.14 + 2.0, iz * 0.14, 1.05 + seed * 0.01) * 0.03
					wr = clamp(math.floor(waterR * (1.0 + wJit)), 0, 255)
					wg = clamp(math.floor(waterG * (1.0 + wJit)), 0, 255)
					wb = clamp(math.floor(waterB * (1.0 + wJit)), 0, 255)
					local wa = clamp(math.floor(wr * ambience), 0, 255)
					local wga = clamp(math.floor(wg * ambience), 0, 255)
					local wba = clamp(math.floor(wb * ambience), 0, 255)
					local maxWAmb = math.max(wa, math.max(wga, wba))
					local wdr = clamp(math.floor(wr * directional), 0, 255 - maxWAmb)
					local wdg = clamp(math.floor(wg * directional), 0, 255 - maxWAmb)
					local wdb = clamp(math.floor(wb * directional), 0, 255 - maxWAmb)
					wr = clamp(math.floor(wdr * colorIdx / 256) + wa, 0, 255)
					wg = clamp(math.floor(wdg * colorIdx / 256) + wga, 0, 255)
					wb = clamp(math.floor(wdb * colorIdx / 256) + wba, 0, 255)
				end

				for iy = 0, ytop do
					local r, g, b = fr, fg, fb
					if waterlayers > 0 and iy < waterlayers then
						r, g, b = wr, wg, wb
					end
					local palIdx = palette:match(r, g, b)
					volume:setVoxel(mins.x + ix, mins.y + iy, mins.z + iz, palIdx)
				end
			end
		end
	end
end
