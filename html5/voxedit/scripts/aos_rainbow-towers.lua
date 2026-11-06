local stb_aos = require "modules/stb_aos"

-- pillar data
local pillar = {}
local num_pillars = 0

-- Function to draw a pillar
local function draw_pillar(x, y, z, col)
	local r = 6
	local r2 = math.floor((36 + 49) / 2)

	-- draw a larger base to make it harder to destroy, and if
	-- they destroy up higher to make it faster, it still leaves
	-- a nice high platform
	stb_aos.draw_sphere(x, y, stb_aos.height[x][y], 12, col)

	-- Draw the main support
	for i = -r, r do
		for j = -r, r do
			if i * i + j * j <= r2 then
				local k = stb_aos.height[x + i][y + j] + 1
				stb_aos.draw_column(x + i, y + j, k, z, col)
			end
		end
	end

	-- Draw the platform
	stb_aos.draw_sphere_slice(x, y, z, 9, col, -3, 0)

	-- Draw a little thing to hide behind
	stb_aos.draw_sphere_slice(x, y, z + 1, 8, col, 0, 0)
	stb_aos.draw_sphere_slice(x, y, z + 1, 7, 0, 0, 0)

	-- Adjust lighting based on distance
	for i = -9, 9 do
		for j = -9, 9 do
			local bright1 = stb_aos.stb_linear_remap(i, -9, 9, 0.5, 1)
			local bright2 = math.sqrt(i * i + j * j)
			bright1 = stb_aos.stb_clamp(bright1, 0.5, 1)
			bright2 = stb_aos.stb_linear_remap(bright2, 0, 9, 0.5, 1)
			bright2 = stb_aos.stb_clamp(bright2, 0.5, 1)
			stb_aos.color[x + i][y + j][z] = stb_aos.light_color(col, stb_aos.stb_clamp(bright1 * bright2, 0.35, 1))
		end
	end
end

-- Function to create a pillar
local function make_pillar(x, y, type)
	local colors = {
		{255, 0, 0},
		{255, 165, 0},
		{255, 255, 0},
		{0, 192, 0},
		{64, 64, 255},
		{120, 40, 255},
		{200, 0, 255}
	}

	local h = stb_aos.height[x][y] + 32
	if h > 60 then
		h = 60
	end

	pillar[num_pillars + 1] = {
		type = type,
		x = x,
		y = y,
		h = h,
		c = colors[num_pillars + 1]
	}
	num_pillars = num_pillars + 1
end

-- Function to place pillars while avoiding proximity to existing ones
local function place_pillars(max_pillars)
	local num_attempts = 0
	while num_attempts < 5000 and num_pillars < max_pillars do
		local i = math.floor(stb_aos.stb_lerp(stb_aos.stb_frand(), 128 + 32, 384 + 32))

		-- Pillars are more likely to be near the center vertically
		local t
		if (i < 256 - 64 or i > 256 + 64) then
			t = 90
		else
			t = 70
		end

		local j
		if ((stb_aos.stb_rand() % 100) < t) then
			j = stb_aos.rnd(128 + 48, 384 - 48)
		else
			j = stb_aos.rnd(128 - 32, 384 + 32)
		end

		-- Check if the new pillar is too close to any existing pillars
		local too_close = false
		for k = 1, num_pillars do
			local dx = pillar[k].x - i
			local dy = pillar[k].y - j
			if dx * dx + dy * dy < 100 * 100 then
				too_close = true
				break
			end
		end

		if not too_close then
			make_pillar(i, j, 0)
		end

		num_attempts = num_attempts + 1
	end
end

-- Function to draw all the pillars
local function add_pillars(max_pillars)
	place_pillars(max_pillars)
	for n = 1, num_pillars do
		-- Combine the RGB components into a 32-bit color
		local c = (pillar[n].c[1] * 65536) + (pillar[n].c[2] * 256) + pillar[n].c[3] + 0x80000000
		local i = pillar[n].x
		local j = pillar[n].y
		local k = pillar[n].h
		draw_pillar(i, j, k, c)
	end
end

local function build_ground()
	stb_aos.grid_random(
		stb_aos.temp,
		2,
		2,
		0,
		6,
		0,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_random(
		stb_aos.data,
		4,
		4,
		0,
		16,
		0,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		8,
		8,
		0,
		20,
		0,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		16,
		16,
		0,
		12,
		0,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		32,
		32,
		0,
		8,
		0.5,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		64,
		64,
		0,
		3,
		0,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)

	for i = 0, stb_aos.MAP_X - 1 do
		for j = 0, stb_aos.MAP_Y - 1 do
			stb_aos.data[i][j] = stb_aos.temp[i][j]
		end
	end

	stb_aos.grid_minimize(stb_aos.data)
	stb_aos.grid_limit(stb_aos.data, 36)

	local water_height = stb_aos.stb_lerp(stb_aos.stb_frand(), -1, 4)
	for i = 0, stb_aos.MAP_X - 1 do
		for j = 0, stb_aos.MAP_Y - 1 do
			stb_aos.data[i][j] = stb_aos.data[i][j] - water_height
		end
	end

	stb_aos.add_shore_detail()

	for i = 0, stb_aos.MAP_X - 1 do
		for j = 0, stb_aos.MAP_Y - 1 do
			local w_raw = stb_aos.stb_linear_remap(stb_aos.data[i][j], 0, 4, 1, 0)
			local w = stb_aos.smoothstep(w_raw)
			w = stb_aos.stb_lerp(w, 0.25, 1)
			stb_aos.data[i][j] = stb_aos.data[i][j] + w * stb_aos.temp[i][j]
		end
	end

	stb_aos.build_height_array()
	stb_aos.add_lighting_detail()
	stb_aos.compute_ground_lighting(4, 24)
end

function description()
	return "Generates a rainbow tower world."
end

function main(node, _, _)
	build_ground()
	add_pillars(7)

	stb_aos.generate_voxels(node)
end
