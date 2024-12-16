--
-- This is a lua conversion of the aos map generators from
-- https://silverspaceship.com/aosmap/
--
-- BEWARE: there might be off-by-one errors in the conversion
--

local module = {}
local vol = require "modules.volume"

-- Constants and masks
module.MAP_X = 512
module.MAP_Y = 512
module.MAP_Z = 64

module.MASK_X = module.MAP_X - 1
module.MASK_Y = module.MAP_Y - 1

-- ground data
module.map = {}
module.color = {}
module.data = {}
module.temp = {}
module.temp2 = {}
module.height = {}

for i = 0, module.MAP_X - 1 do
	module.map[i] = {}
	module.color[i] = {}
	module.data[i] = {}
	module.temp[i] = {}
	module.temp2[i] = {}
	module.height[i] = {}
	for j = 0, module.MAP_Y - 1 do
		module.map[i][j] = {}
		module.color[i][j] = {}
	end
end

-- pillar data
module.pillar = {}
module.num_pillars = 0

-- Define the pillar_data structure (using a table in Lua)
module.pillar_data = {
	x = 0,
	y = 0,
	h = 0,
	c = {0, 0, 0},
	type = 0
}

-- water data
module.water = {}
for i = 0, module.MAP_X - 1 do
	module.water[i] = {}
	for j = 0, module.MAP_Y - 1 do
		module.water[i][j] = 0
	end
end

-- building data

module.OUTER_WALL_HEIGHT = 5
module.OUTER_WALL_COLOR = 0x80404040
module.STORY1_HEIGHT = 12
module.STORY2_HEIGHT = 28
module.CATWALK_COLOR = 0x80c0a060
module.MOAT_SIZE = 6

function module.generate_voxels(node)
	local region = g_region.new(0, 0, 0, module.MAP_X - 1, module.MAP_Z - 1, module.MAP_Y - 1)
	local newLayer = g_scenegraph.new("aos", region)
	local newVolume = newLayer:volume()
	newLayer:setPalette(node:palette())

	g_log.debug("map generation done")
	local visitor = function(_, x, y, z)
		local xmap = module.map[x]
		local ymap = xmap and xmap[z]
		local zmap = ymap and ymap[y]
		if zmap == nil then
			return
		end
		if zmap == false then
			return
		end
		local c = module.color[x][z][y]
		local a = (c >> 24) & 0xFF
		if a == 0 then
			return
		end
		local r = (c >> 16) & 0xFF
		local g = (c >> 8) & 0xFF
		local b = c & 0xFF
		local newcolor = node:palette():match(r, g, b)
		newVolume:setVoxel(x, y, z, newcolor)
	end
	g_log.debug("placing voxels")
	vol.visitYXZ(newLayer, region, visitor)
end

-- Linear interpolation
function module.stb_lerp(t, a, b)
	return a + t * (b - a)
end

-- Reverse interpolation (unlerp)
function module.stb_unlerp(t, a, b)
	return (t - a) / (b - a)
end

-- Clamp a value between a minimum and maximum
function module.stb_clamp(x, xmin, xmax)
	if x < xmin then
		return xmin
	elseif x > xmax then
		return xmax
	end
	return x
end

function module.stb_linear_remap(x, x_min, x_max, out_min, out_max)
	return module.stb_lerp(module.stb_unlerp(x, x_min, x_max), out_min, out_max)
end

-- Utility functions
function module.compute_water_height(water_squares_desired)
	local wmin, wmax = -32, 32
	local water_height = 0

	while wmin < wmax - (1.0 / 32) do
		local water = 0
		water_height = (wmin + wmax) / 2

		for i = 384, module.MAP_X - 1 do
			for j = 128, 383 do
				if module.data[i][j] <= water_height then
					water = water + 1
				end
			end
		end

		if water > water_squares_desired + 32 then
			wmax = water_height
		elseif water < water_squares_desired - 32 then
			wmin = water_height
		else
			break
		end
	end

	for i = 0, module.MAP_X - 1 do
		for j = 0, module.MAP_Y - 1 do
			module.data[i][j] = module.data[i][j] - water_height + 0.99
		end
	end
end

function module.draw_column(x, y, z0, z1, c)
	if x < 0 or x >= module.MAP_X or y < 0 or y >= module.MAP_Y then
		return
	end
	if z0 < 0 or z1 < 0 or z0 >= module.MAP_Z or z1 >= module.MAP_Z then
		return
	end
	local solid = (c ~= 0)
	for z = z0, z1 do
		module.map[x][y][z] = solid
		module.color[x][y][z] = c
	end
end

function module.rnd(x, y)
	assert(x <= y, "Invalid range for rnd")
	return math.random(x, y)
end

function module.smoothstep(v)
	if v < 0 then
		return 0
	end
	if v > 1 then
		return 1
	end
	return 3 * v * v - 2 * v * v * v
end

function module.smooth_squarestep(v)
	if v < 0 then
		return 0
	end
	if v > 1 then
		return 1
	end
	return v * v
end

function module.smooth_cubestep(v)
	if v < 0 then
		return 0
	end
	if v > 1 then
		return 1
	end
	return v * v * v
end

-- Given a number from 0..1, make sure it wraps around smoothly
function module.match_edges(x)
	if x < 1.0 / 16 then
		return module.stb_linear_remap(x, 0, 1.0 / 16, 0.5, 1.0 / 16)
	elseif x >= 15.0 / 16 then
		return module.stb_linear_remap(x, 15.0 / 16, 1, 15.0 / 16, 0.5)
	end
	return x
end

function module.grid_random(dest, x_samples, y_samples, r0, r1, w, x_origin, y_origin, z)
	local x_wrap = x_samples
	local y_wrap = y_samples
	local x_scale = x_wrap / module.MAP_X
	local y_scale = y_wrap / module.MAP_Y

	for i = 0, module.MAP_X - 1 do
		for j = 0, module.MAP_Y - 1 do
			local x = x_scale * i + x_origin
			local y = y_scale * j + y_origin
			local p = g_noise.noise3(x, y, z) * 0.5 + 0.5
			p = p / 1.2
			p = (p + 1) / 2
			p = module.stb_lerp(w, p, p * p * p)
			dest[i][j] = module.stb_lerp(p, r0, r1)
		end
	end
end

function module.grid_minimize(dest)
	local f = dest[0][0]
	for i = 0, module.MAP_X - 1 do
		for j = 0, module.MAP_Y - 1 do
			if dest[i][j] < f then
				f = dest[i][j]
			end
		end
	end

	for i = 0, module.MAP_X - 1 do
		for j = 0, module.MAP_Y - 1 do
			dest[i][j] = dest[i][j] - f
		end
	end
end

function module.grid_limit(dest, limit)
	local f = dest[0][0]
	for i = 0, module.MAP_X - 1 do
		for j = 0, module.MAP_Y - 1 do
			if dest[i][j] > f then
				f = dest[i][j]
			end
		end
	end

	if f > limit then
		local scale = limit / f
		for i = 0, module.MAP_X - 1 do
			for j = 0, module.MAP_Y - 1 do
				dest[i][j] = dest[i][j] * scale
			end
		end
	end
end

function module.grid_addeq_scale(dest, src, scale)
	for i = 0, module.MAP_X - 1 do
		for j = 0, module.MAP_Y - 1 do
			dest[i][j] = dest[i][j] + src[i][j] * scale
		end
	end
end

function module.build_height_array()
	for i = 0, module.MAP_X - 1 do
		for j = 0, module.MAP_Y - 1 do
			local z = math.floor(module.data[i][j])
			if z < 0 then
				z = 0
			end
			if z > module.MAP_Z - 2 then
				z = module.MAP_Z - 2
			end

			module.height[i][j] = z
			for k = 0, module.MAP_Z - 1 do
				module.map[i][j][k] = (k <= z)
			end
		end
	end
end

function module.add_shore_detail()
	module.grid_random(module.temp, 64, 64, -0.5, 0.5, 0, 0, 0, module.stb_frand() * module.MAP_Z)
	module.grid_random(module.temp2, 128, 128, 0, 1, 0, 0, 0, module.stb_frand() * module.MAP_Z)
	module.grid_addeq_scale(module.temp, module.temp2, 1)
	module.grid_random(module.temp2, 256, 256, 0, -1, 0, 0, 0, module.stb_frand() * module.MAP_Z)
	module.grid_addeq_scale(module.temp, module.temp2, 1)
	module.grid_addeq_scale(module.data, module.temp, 1)
end

function module.add_lighting_detail()
	local w1, w2, w3 = 1.0, 1.0 / 5, 1.0 / 12
	local scale = 2
	module.grid_random(module.temp, 32, 32, 0, w1, 0, 0, 0, module.stb_frand() * module.MAP_Z)
	module.grid_random(module.temp2, 128, 128, 0, w2, 0, 0, 0, module.stb_frand() * module.MAP_Z)
	module.grid_addeq_scale(module.temp, module.temp2, 1)
	module.grid_random(module.temp2, 256, 256, 0, w3, 0, 0, 0, module.stb_frand() * module.MAP_Z)
	module.grid_addeq_scale(module.temp, module.temp2, 1)
	module.grid_addeq_scale(module.data, module.temp, scale)
end

function module.stb_rand()
	return math.random(0, 4294967295) -- 32 bits
end

function module.stb_frand()
	local high = math.random(0, 65535) -- 16 bits
	local low = math.random(0, 65535) -- 16 bits
	return ((high << 16) | low) / (2 ^ 32)
end

function module.draw_sphere(x, y, z, r, c)
	local solid = (c ~= 0)
	local r2 = r * r + ((2 * r + 1) / 2)

	for i = -r, r do
		for j = -r, r do
			for k = -r, r do
				if i * i + j * j + k * k < r2 then
					if z + k >= 0 and z + k < module.MAP_Z then
						local a = (x + i) & module.MASK_X
						local b = (y + j) & module.MASK_Y
						module.map[a][b][z + k] = solid
						module.color[a][b][z + k] = c
					end
				end
			end
		end
	end
end

function module.draw_sphere_slice(x, y, z, r, c, k0, k1)
	local solid = (c ~= 0)
	local r2 = r * r + ((2 * r + 1) / 2)

	for i = -r, r do
		for j = -r, r do
			for k = k0, k1 do
				if i * i + j * j + k * k < r2 then
					if z + k >= 0 and z + k < module.MAP_Z then
						local a = (x + i) & module.MASK_X
						local b = (y + j) & module.MASK_Y
						module.map[a][b][z + k] = solid
						module.color[a][b][z + k] = c
					end
				end
			end
		end
	end
end

function module.draw_rotated_ellipse(x, y, z, theta, rx, ry, rz, c)
	local sn, cs = math.sin(theta), math.cos(theta)
	local solid = (c ~= 0)
	local bounds = math.max(rx, ry)

	for k = -rz, rz do
		for i = -bounds, bounds do
			for j = -bounds, bounds do
				local dx = (cs * i + sn * j) / rx
				local dy = (-sn * i + cs * j) / ry
				local dz = k / rz

				if dx * dx + dy * dy + dz * dz < 1 then
					if z + k >= 0 and z + k < module.MAP_Z then
						local a = (x + i) & module.MASK_X
						local b = (y + j) & module.MASK_Y
						module.map[a][b][z + k] = solid
						module.color[a][b][z + k] = c
					end
				end
			end
		end
	end
end

function module.draw_partial_sphere_line(x0, y0, z0, x1, y1, z1, rad, k0, k1, c0, c1, a)
	local dx, dy = math.abs(x0 - x1) + 1, math.abs(y0 - y1) + 1
	local d = math.max(dx, dy)

	for i = 0, d do
		local t = module.stb_linear_remap(i, 0, d, 0.0, 1.0)
		local x = math.floor(x0 + (x1 - x0) * t + 0.5)
		local y = math.floor(y0 + (y1 - y0) * t + 0.5)
		local z = math.floor(z0 + (z1 - z0) * t + 0.5)

		local r = module.stb_linear_remap(i, 0, d, c0[1], c1[1])
		local g = module.stb_linear_remap(i, 0, d, c0[2], c1[2])
		local b = module.stb_linear_remap(i, 0, d, c0[3], c1[3])

		module.draw_sphere_slice(x, y, z, rad, (r * 65536 + g * 256 + b + (a << 24)), k0, k1)
	end
end

function module.translate(dest, src, dx, dy)
	for i = 0, module.MAP_X - 1 do
		for j = 0, module.MAP_Y - 1 do
			dest[i][j] = src[(i + dx) & module.MASK_X][(j + dy) & module.MASK_Y]
		end
	end
end

function module.compute_ground_lighting(green_height, white_height)
	for i = 0, module.MAP_X - 1 do
		for j = 0, module.MAP_Y - 1 do
			local height_color = module.stb_linear_remap(module.data[i][j], green_height, white_height, 0, 1)
			local dx = module.data[i][j] - module.data[(i - 1) & module.MASK_X][j]
			local dy = module.data[i][j] - module.data[i][(j - 1) & module.MASK_X]
			local slope = dx * dx + dy * dy
			local slope_color = module.stb_linear_remap(slope, 0, 1.6, 0, 1)
			height_color = module.smoothstep(height_color)
			slope_color = module.smoothstep(slope_color)

			local brightness = (0.6 * dx + 0.8) / math.sqrt(slope + 1)
			brightness = module.stb_clamp(brightness, 0, 1)
			brightness = module.stb_lerp(brightness, 0.3, 1.0)
			brightness = module.stb_clamp(brightness, 0, 1)

			local r, g, b, a
			if module.height[i][j] == 0 then
				height_color = module.stb_linear_remap(module.data[i][j], -0.5, 2.5, 1, 0)
				height_color = module.stb_clamp(height_color, 0, 1)
				r = module.stb_lerp(height_color, 144, 130)
				g = module.stb_lerp(height_color, 160, 170)
				b = module.stb_lerp(height_color, 104, 200)
				brightness = 1
			else
				r = module.stb_lerp(height_color, 144, 220)
				g = module.stb_lerp(height_color, 160, 220)
				b = module.stb_lerp(height_color, 104, 220)
				r = module.stb_lerp(slope_color, r, 120)
				g = module.stb_lerp(slope_color, g, 120)
				b = module.stb_lerp(slope_color, b, 120)
			end

			r = math.floor(r * brightness)
			g = math.floor(g * brightness)
			b = math.floor(b * brightness)
			a = 0x80

			local c = (r << 16) + (g << 8) + b + (a << 24)
			for k = 0, module.MAP_Z - 1 do
				module.color[i][j][k] = c
			end
		end
	end
end

-- Function to create a pillar
function module.make_pillar(x, y, type)
	local colors = {
		{255, 0, 0},
		{255, 165, 0},
		{255, 255, 0},
		{0, 192, 0},
		{64, 64, 255},
		{120, 40, 255},
		{200, 0, 255}
	}

	local h = module.height[x][y] + 32
	if h > 60 then
		h = 60
	end

	module.pillar[module.num_pillars + 1] = {
		type = type,
		x = x,
		y = y,
		h = h,
		c = colors[module.num_pillars + 1]
	}
	module.num_pillars = module.num_pillars + 1
end

-- Function to adjust color brightness
function module.light_color(color, brightness)
	local r = (color >> 16) & 0xff
	local g = (color >> 8) & 0xff
	local b = (color >> 0) & 0xff

	r = math.floor(r * brightness)
	g = math.floor(g * brightness)
	b = math.floor(b * brightness)

	return (color & 0xFF000000) + r * 65536 + g * 256 + b
end

-- Function to draw a pillar (stubbed out for illustration)
function module.draw_pillar(x, y, z, col)
	local r = 6
	local r2 = math.floor((36 + 49) / 2)

	-- Draw a larger base (using a stubbed function for illustration)
	module.draw_sphere(x, y, module.height[x][y], 12, col)

	-- Draw the main support
	for i = -r, r do
		for j = -r, r do
			if i * i + j * j <= r2 then
				local k = module.height[x + i][y + j] + 1
				module.draw_column(x + i, y + j, k, z, col)
			end
		end
	end

	-- Draw the platform
	module.draw_sphere_slice(x, y, z, 9, col, -3, 0)

	-- Draw a little thing to hide behind
	module.draw_sphere_slice(x, y, z + 1, 8, col, 0, 0)
	module.draw_sphere_slice(x, y, z + 1, 7, 0, 0, 0)

	-- Adjust lighting based on distance
	for i = -9, 9 do
		for j = -9, 9 do
			local bright1 = module.stb_linear_remap(i, -9, 9, 0.5, 1)
			local bright2 = math.sqrt(i * i + j * j)
			bright1 = module.stb_clamp(bright1, 0.5, 1)
			bright2 = module.stb_linear_remap(bright2, 0, 9, 0.5, 1)
			bright2 = module.stb_clamp(bright2, 0.5, 1)
			module.color[x + i][y + j][z] = module.light_color(col, module.stb_clamp(bright1 * bright2, 0.35, 1))
		end
	end
end

-- Function to place pillars while avoiding proximity to existing ones
function module.place_pillars(max_pillars)
	local num_attempts = 0
	while num_attempts < 5000 and module.num_pillars < max_pillars do
		local i = math.floor(module.stb_lerp(module.stb_frand(), 128 + 32, 384 + 32))

		-- Pillars are more likely to be near the center vertically
		local j
		if (module.stb_rand() % 100) < ((i < 256 - 64 or i > 256 + 64) and 90 or 70) then
			j = module.rnd(128 + 48, 384 - 48)
		else
			j = module.rnd(128 - 32, 384 + 32)
		end

		-- Check if the new pillar is too close to any existing pillars
		local too_close = false
		for k = 1, module.num_pillars do
			local pillar = module.pillar[k]
			local dx = pillar.x - i
			local dy = pillar.y - j
			if dx * dx + dy * dy < 100 * 100 then
				too_close = true
				break
			end
		end

		if not too_close then
			module.make_pillar(i, j, 0)
		end

		num_attempts = num_attempts + 1
	end
end

-- Function to draw all the pillars
function module.add_pillars(max_pillars)
	module.place_pillars(max_pillars)
	for n = 1, module.num_pillars do
		-- Combine the RGB components into a 32-bit color
		local pillar = module.pillar[n]
		local c = (pillar.c[1] * 65536) + (pillar.c[2] * 256) + pillar.c[3] + 0x80000000
		local i = pillar.x
		local j = pillar.y
		local k = pillar.h
		module.draw_pillar(i, j, k, c)
	end
end

function module.place_trees()
	for i = 6, module.MAP_X - 7, 7 do
		local density = module.stb_linear_remap(i, 0, module.MAP_X, 1.5, -0.5)
		density = module.smoothstep(density) * 0.75
		for j = 6, module.MAP_Y - 7, 7 do
			if module.stb_frand() < density then
				-- Place something
				local x = i + (module.stb_rand() % 4)
				local y = j + (module.stb_rand() % 4)
				local dx = module.data[x][y] - module.data[x + 1][y]
				local dy = module.data[x][y] - module.data[x][y - 1]
				if dx * dx + dy * dy < 0.35 and module.height[x][y] > 1 and module.height[x][y] < 63 - 4 then
					local z = module.height[x][y] + 1
					if module.stb_frand() < density * 0.75 and z + 7 < 64 then
						-- Tree trunk
						local trunk_color = 0x80604020
						module.draw_column(x, y, z, z + 3, trunk_color)

						-- Tree crown
						local crown_color = 0x80507040
						if (module.stb_rand() & 4) ~= 0 then
							z = z + 2
						else
							z = z + 3
						end

						module.draw_column(x - 2, y - 1, z, z, crown_color)
						module.draw_column(x - 2, y + 0, z, z, crown_color)
						module.draw_column(x - 2, y + 1, z, z, crown_color)

						module.draw_column(x + 2, y - 1, z, z, crown_color)
						module.draw_column(x + 2, y + 0, z, z, crown_color)
						module.draw_column(x + 2, y + 1, z, z, crown_color)

						module.draw_column(x - 1, y - 2, z, z, crown_color)
						module.draw_column(x + 0, y - 2, z, z, crown_color)
						module.draw_column(x + 1, y - 2, z, z, crown_color)

						module.draw_column(x - 1, y + 2, z, z, crown_color)
						module.draw_column(x + 0, y + 2, z, z, crown_color)
						module.draw_column(x + 1, y + 2, z, z, crown_color)

						module.draw_column(x - 1, y - 1, z, z + 1, crown_color)
						module.draw_column(x - 1, y + 0, z, z + 2, crown_color)
						module.draw_column(x - 1, y + 1, z, z + 1, crown_color)

						module.draw_column(x + 1, y - 1, z, z + 1, crown_color)
						module.draw_column(x + 1, y + 0, z, z + 2, crown_color)
						module.draw_column(x + 1, y + 1, z, z + 1, crown_color)

						module.draw_column(x - 1, y - 1, z, z + 1, crown_color)
						module.draw_column(x + 0, y - 1, z, z + 2, crown_color)
						module.draw_column(x + 1, y - 1, z, z + 1, crown_color)

						module.draw_column(x - 1, y + 1, z, z + 1, crown_color)
						module.draw_column(x + 0, y + 1, z, z + 2, crown_color)
						module.draw_column(x + 1, y + 1, z, z + 1, crown_color)

						module.draw_column(x, y, z, z + 3, crown_color)
					elseif module.stb_frand() < density * 0.75 then
						-- Smaller bush-like structure
						local bush_color = 0x80206020
						module.draw_column(x, y, z, z + 1 + (module.stb_rand() % 2), bush_color)
						module.draw_column(x + 1, y, z, z + 1 + (module.stb_rand() % 2), bush_color)
						module.draw_column(x, y + 1, z, z + 1 + (module.stb_rand() % 2), bush_color)
						module.draw_column(x + 1, y + 1, z, z + 1 + (module.stb_rand() % 2), bush_color)
					else
						-- Small single pillar
						local single_pillar_color = 0x80204020
						module.draw_column(x, y, z, z + 1 + (module.stb_rand() % 2), single_pillar_color)
					end
				end
			end
		end
	end
end

function module.make_water()
	-- Reset water values
	for i = 0, module.MAP_X - 1 do
		for j = 0, module.MAP_Y - 1 do
			module.water[i][j] = (module.water[i][j] ~= 0) and 0 or 255
		end
	end

	-- Measure distance to water
	for k = 0, 63 do
		for i = 0, module.MAP_X - 1 do
			for j = 0, module.MAP_Y - 1 do
				if module.water[i][j] == k then
					local x0 = (i - 1) % module.MAP_X
					local x1 = (i + 1) % module.MAP_X
					local y0 = (j - 1) % module.MAP_Y
					local y1 = (j + 1) % module.MAP_Y

					if module.water[i][y0] == 255 then
						module.water[i][y0] = k + 1
					end
					if module.water[i][y1] == 255 then
						module.water[i][y1] = k + 1
					end
					if module.water[x0][j] == 255 then
						module.water[x0][j] = k + 1
					end
					if module.water[x1][j] == 255 then
						module.water[x1][j] = k + 1
					end
				end
			end
		end
	end

	-- Lower ground around water to match water
	for i = 0, module.MAP_X - 1 do
		local clamp, reach
		local fake_i = i
		if i < 32 then
			fake_i = module.stb_linear_remap(i, 0, 32, 32, module.MAP_X)
		end
		clamp = module.stb_linear_remap(fake_i, 32, module.MAP_X, 1.0, 1.1)
		reach = module.stb_linear_remap(fake_i, 32, module.MAP_X, 12, 24)

		for j = 0, module.MAP_Y - 1 do
			if module.water[i][j] < 255 then
				local t = module.stb_linear_remap(module.water[i][j], 0, reach, 0, 1)
				if t < 1 then
					t = module.stb_clamp(t, 0, clamp)
					t = t ^ 0.7
					module.data[i][j] = module.data[i][j] * t
				end
			end
		end
	end
end

function module.draw_pool(x, y, r)
	local r2 = r * r + math.floor((2 * r + 1) / 2)
	for i = -r, r do
		for j = -r, r do
			if i * i + j * j <= r2 then
				local a = math.floor(x + i) & module.MASK_X
				local b = math.floor(y + j) & module.MASK_Y
				module.water[a][b] = 1
			end
		end
	end
end

function module.draw_river_segment(x0, y0, x1, y1, r)
	local n = math.max(math.abs(x1 - x0), math.abs(y1 - y0)) + math.random(0, 4)
	for i = 0, n do
		local t = module.stb_linear_remap(i, 0, n, 0.0, 1.0)
		local pool_x = module.stb_lerp(t, x0, x1) + math.random(-1, 1)
		local pool_y = module.stb_lerp(t, y0, y1) + math.random(-1, 1)
		module.draw_pool(pool_x, pool_y, r)
	end
end

function module.draw_recursive_river(x0, y0, x1, y1, r, s)
	local x = math.floor((x0 + x1) / 2)
	local y = math.floor((y0 + y1) / 2)
	local dx = math.abs(x - x0)
	local dy = math.abs(y - y0)
	local dist = math.max(dx, dy) + math.floor(math.min(dx, dy) / 2)
	local displace = math.floor(dist / 8)

	if displace > 1 then
		x = x + math.random(-displace, displace)
		y = y + math.random(-displace, displace)
	end

	if dist < 4 then
		module.draw_river_segment(x0, y0, x, y, r)
		module.draw_river_segment(x, y, x1, y1, r)
	else
		module.draw_recursive_river(x0, y0, x, y, r, s + 1)
		if s == 0 then
			module.draw_recursive_river(x, y, x1, y1, r - 1, s + 1)
			local offset = math.random(24, 47)
			if math.random(0, 1) == 1 then
				offset = -offset
			end
			module.draw_recursive_river(x, y, x1, y1 + offset, r - 1, s + 1)
		else
			module.draw_recursive_river(x, y, x1, y1, r, s + 1)
		end
	end
end

function module.draw_random_river(x, y, dx, dy, r)
	local y2 = module.stb_linear_remap(module.MAP_X, x, x + dx * 100, y, y + dy * 150)
	module.draw_recursive_river(x, y, module.MAP_X, y2, r, 0)
end

function module.draw_hstair(x, y, z0, z1, d, w, color)
	local clear_top = 8
	local z = z1 - 1
	while z > z0 do
		for e = -w, w do
			local h = z - 3
			if h < z0 then
				h = z0
			end
			module.draw_column(x, y + e, h, z, color)
		end
		module.draw_column(x, y, z + 1, z + clear_top, 0)
		clear_top = clear_top - 1
		if clear_top < 6 then
			clear_top = 6
		end
		z = z - 2
		x = x + d
		if x >= module.MAP_X or x < 0 then
			return
		end
	end
end

function module.draw_hwall(x0, x1, y, z0, z1, color)
	for x = x0, x1 do
		module.draw_column(x, y, z0, z1, color)
	end
end

function module.draw_vwall(x, y0, y1, z0, z1, color)
	for y = y0, y1 do
		module.draw_column(x, y, z0, z1, color)
	end
end

function module.draw_rect_wall(x0, y0, x1, y1, h, color)
	module.draw_hwall(x0, x1, y0, 0, h, color)
	module.draw_hwall(x0, x1, y1, 0, h, color)
	module.draw_vwall(x0, y0, y1, 0, h, color)
	module.draw_vwall(x1, y0, y1, 0, h, color)
end

function module.draw_rect_wall_partial(x0, y0, x1, y1, h, color)
	local cycle = 0
	for x = x0, x1 do
		if cycle % 10 < 3 then
			module.draw_column(x, y0, 0, h, color)
			module.draw_column(x, y1, 0, h, color)
		end
		cycle = cycle + 1
	end
	for y = y0, y1 do
		if cycle % 10 < 3 then
			module.draw_column(x0, y, 0, h, color)
			module.draw_column(x1, y, 0, h, color)
		end
		cycle = cycle + 1
	end
end

function module.draw_box(x0, y0, x1, y1, z0, z1, color)
	for x = x0, x1 do
		for y = y0, y1 do
			module.draw_column(x, y, z0, z1, color)
		end
	end
end

-- a pile is a collection meant for building materials
function module.draw_pile(x, y, z, color)
	module.draw_box(x - 2, y - 2, x + 2, y + 2, z, z + 3, color)
end

function module.make_building(x0, y0, x1, y1, type)
	-- make foundation
	module.draw_box(x0 - 1, y0 - 1, x1 + 2, y1 + 2, 0, 4, module.BUILDING_COLOR)
	module.draw_box(x0, y0, x1 + 1, y1 + 1, 0, 5, module.BUILDING_COLOR)
	module.draw_box(x0, y0, x1 + 1, y1 + 1, 6, 32, 0)

	-- make wall
	module.draw_rect_wall(x0, y0, x1 + 1, y1 + 1, module.STORY1_HEIGHT, module.BUILDING_COLOR)
	module.draw_rect_wall(x0 + 1, y0 + 1, x1, y1, module.STORY1_HEIGHT, module.BUILDING_COLOR)

	if type == 0 then
		-- split it up 3x3, and make the central region a tower
		local x2 = math.floor((x0 * 2 + x1) / 3 + module.rnd(-6, 6))
		local x3 = math.floor((x0 + x1 * 2) / 3 + module.rnd(-6, 6))
		local y2 = math.floor((y0 * 2 + y1) / 3 + module.rnd(-6, 6))
		local y3 = math.floor((y0 + y1 * 2) / 3 + module.rnd(-6, 6))

		local xpos = {x0, x2, x3, x1}
		local ypos = {y0, y2, y3, y1}

		-- build the internal walls
		module.draw_hwall(x0, x1, y2, 5, module.STORY1_HEIGHT, module.BUILDING_COLOR)
		module.draw_hwall(x0, x1, y2 + 1, 5, module.STORY1_HEIGHT, module.BUILDING_COLOR)

		module.draw_hwall(x0, x1, y3, 5, module.STORY1_HEIGHT, module.BUILDING_COLOR)
		module.draw_hwall(x0, x1, y3 + 1, 5, module.STORY1_HEIGHT, module.BUILDING_COLOR)

		module.draw_vwall(x2, y0, y1, 5, module.STORY1_HEIGHT, module.BUILDING_COLOR)
		module.draw_vwall(x2 + 1, y0, y1, 5, module.STORY1_HEIGHT, module.BUILDING_COLOR)

		module.draw_vwall(x3, y0, y1, 5, module.STORY1_HEIGHT, module.BUILDING_COLOR)
		module.draw_vwall(x3 + 1, y0, y1, 5, module.STORY1_HEIGHT, module.BUILDING_COLOR)

		-- build a doorway in every direction
		for i = 1, 4 do
			for j = 1, 4 do
				local n
				if j == 1 or j == 4 then
					n = 100
				else
					n = 70
				end
				if i < 4 and math.random(0, 100) < n then
					-- draw gap in a horizontal wall
					local x = module.rnd(xpos[i] + 2, xpos[i + 1] - 2)
					local y = ypos[j]
					module.draw_box(x, y, x + 1, y + 1, 6, 10, 0)
				end
				if i == 1 or i == 4 then
					n = 100
				else
					n = 80
				end
				if j < 4 and math.random(0, 100) < n then
					-- draw gap in a vertical wall
					local x = xpos[i]
					local y = module.rnd(ypos[j] + 2, ypos[j + 1] - 2)
					module.draw_box(x, y, x + 1, y + 1, 6, 10, 0)
				end
			end
		end

		local x_catwalk = {}
		local y_catwalk = {}
		for i = 1, 3 do
			x_catwalk[i] = math.floor((xpos[i] + xpos[i + 1]) / 2 + module.rnd(-3, 3))
			y_catwalk[i] = math.floor((ypos[i] + ypos[i + 1]) / 2 + module.rnd(-3, 3))
		end

		-- decide whether to build a roof or not
		for i = 1, 3 do
			for j = 1, 3 do
				if not (i == 2 and j == 2) then
					if math.random(0, 100) < 40 then
						-- build a roof
						module.draw_box(
							xpos[i],
							ypos[j],
							xpos[i + 1],
							ypos[j + 1],
							module.STORY1_HEIGHT - 1,
							module.STORY1_HEIGHT,
							module.BUILDING_COLOR
						)
					else
						-- build catwalks
						local xm = module.rnd(-1, 0)
						local ym = module.rnd(-1, 0)
						local xw = module.rnd(0, 1)
						local yw = module.rnd(0, 1)
						module.draw_box(
							x_catwalk[i] + xm,
							ypos[j] + 2,
							x_catwalk[i] + xw,
							ypos[j + 1] - 1,
							module.STORY1_HEIGHT,
							module.STORY1_HEIGHT,
							module.CATWALK_COLOR
						)
						module.draw_box(
							xpos[i] + 2,
							y_catwalk[j] + ym,
							xpos[i + 1] - 1,
							y_catwalk[j] + yw,
							module.STORY1_HEIGHT,
							module.STORY1_HEIGHT,
							module.CATWALK_COLOR
						)
					end
				end
			end
		end

		-- build very thick walls around the center thing
		module.draw_box(x2 - 2, y2 - 2, x3 + 3, y3 + 3, module.STORY1_HEIGHT, module.STORY1_HEIGHT + 1, module.BUILDING_COLOR)
		module.draw_box(
			x2 - 1,
			y2 - 1,
			x3 + 2,
			y3 + 2,
			module.STORY1_HEIGHT + 2,
			module.STORY1_HEIGHT + 4,
			module.BUILDING_COLOR
		)
		module.draw_box(x2, y2, x3 + 1, y3 + 1, module.STORY1_HEIGHT + 5, module.STORY1_HEIGHT + 7, module.BUILDING_COLOR)
		module.draw_box(x2 + 1, y2 + 1, x3, y3, module.STORY1_HEIGHT + 8, module.STORY2_HEIGHT, module.BUILDING_COLOR)

		-- dig out the interior of the tower
		module.draw_box(x2 + 2, y2 + 2, x3 - 1, y3 - 1, module.STORY1_HEIGHT - 2, module.STORY2_HEIGHT, 0)
		-- draw the tower floor
		module.draw_box(x2 + 1, y2 + 1, x3, y3, module.STORY1_HEIGHT + 9, module.STORY1_HEIGHT + 11, module.BUILDING_COLOR)

		-- dig a hole in the floor or build stairs
		local x  --  = math.floor((x0 + x1) / 2)
		local y = math.floor((y0 + y1) / 2)

		x = x2 + module.rnd(2, 5)
		module.draw_hstair(x - 1, y, 5, module.STORY1_HEIGHT + 11, 1, 0, 0x8060c060)
	else
		-- create building materials
		for i = 1, 4 do
			for j = 1, 4 do
				module.draw_pile(
					module.stb_linear_remap(i - 1, -0.5, 3.5, x0, x1) + module.rnd(-1, 1),
					module.stb_linear_remap(j - 1, -0.5, 3.5, y0, y1) + module.rnd(-1, 1),
					6,
					0x8060c020
				)
			end
		end

		-- draw partial roof
		for i = 1, 4 do
			local x = module.stb_linear_remap(i - 1, -0.5, 3.5, x0, x1) + module.rnd(-1, 1)
			local y = module.stb_linear_remap(i - 1, -0.5, 3.5, y0, y1) + module.rnd(-1, 1)
			module.draw_box(
				x,
				y0 + 1,
				x + module.rnd(0, 1),
				y1 + 1,
				module.STORY1_HEIGHT,
				module.STORY1_HEIGHT,
				module.CATWALK_COLOR
			)
			module.draw_box(
				x0 + 1,
				y,
				x1,
				y + module.rnd(0, 1),
				module.STORY1_HEIGHT,
				module.STORY1_HEIGHT,
				module.CATWALK_COLOR
			)
		end
	end
end

function module.make_fortress()
	local x0 = 384 + module.MOAT_SIZE + 8
	local x1 = module.MAP_X - module.MOAT_SIZE - 8
	local y0 = 128 + module.MOAT_SIZE + 8
	local y1 = 384 - module.MOAT_SIZE - 8
	local y_split

	-- make outer wall
	module.draw_rect_wall(x0, y0, x1, y1, module.OUTER_WALL_HEIGHT, module.OUTER_WALL_COLOR)

	-- make occasional higher walls
	module.draw_rect_wall_partial(x0, y0, x1, y1, module.OUTER_WALL_HEIGHT + 4, module.OUTER_WALL_COLOR)

	-- make back doors
	module.draw_box(x1 - 1, 256 - 64, x1, 256 - 64, 1, 6, 0)
	module.draw_box(x1 - 1, 256 + 64, x1, 256 + 64, 1, 6, 0)

	-- make front door
	module.draw_box(x0 - 2, 256 - 4, x0 + 2, 256 + 4, 1, 11, 0x80606060)
	module.draw_box(x0 - 4, 256 - 1, x0 + 4, 256 + 1, 3, 6, 0)

	-- make the three buildings
	y_split = y0 + math.floor((y1 - y0) / 3) - 12
	module.make_building(math.floor((x0 + x1) / 2) - 32, y0 + 16, x1 - 48, y_split, 0)

	y_split = y0 + math.floor((y1 - y0) / 3 * 2) + 12
	module.make_building(math.floor((x0 + x1) / 2) - 32, y_split, x1 - 48, y1 - 16, 0)

	module.make_building(
		math.floor((x0 + x1) / 2) - 16,
		y0 + math.floor((y1 - y0) / 3) + 12,
		x1 - 12,
		y0 + math.floor((y1 - y0) / 3 * 2) - 12,
		1
	)
end

return module
