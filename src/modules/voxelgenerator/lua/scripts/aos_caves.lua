local stb_aos = require "modules/stb_aos"
local green_terrain_bias = 0.5
local NUM_PASSAGES = 10
local MAX_PILLARS = 1024
local pillar = {}
local num_pillars = 0
local nearby_pillar = {}

for i = 1, MAX_PILLARS do
	nearby_pillar[i] = {}
	for j = 1, MAX_PILLARS do
		nearby_pillar[i][j] = 0
	end
end

local function build_ground()
	local water_squares_desired = 2048

	stb_aos.grid_random(
		stb_aos.temp,
		4,
		4,
		0,
		24,
		0,
		stb_aos.stb_frand() * stb_aos.MAP_X,
		stb_aos.stb_frand() * stb_aos.MAP_Y,
		stb_aos.stb_frand() * stb_aos.MAP_Z
	)
	stb_aos.grid_random(
		stb_aos.data,
		4,
		4,
		0,
		12,
		0,
		stb_aos.stb_frand() * stb_aos.MAP_X,
		stb_aos.stb_frand() * stb_aos.MAP_Y,
		stb_aos.stb_frand() * stb_aos.MAP_Z
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		16,
		16,
		0,
		12,
		0,
		stb_aos.stb_frand() * stb_aos.MAP_X,
		stb_aos.stb_frand() * stb_aos.MAP_Y,
		stb_aos.stb_frand() * stb_aos.MAP_Z
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		32,
		32,
		0,
		8,
		0,
		stb_aos.stb_frand() * stb_aos.MAP_X,
		stb_aos.stb_frand() * stb_aos.MAP_Y,
		stb_aos.stb_frand() * stb_aos.MAP_Z
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.data = stb_aos.temp

	stb_aos.grid_minimize(stb_aos.data)
	stb_aos.grid_limit(stb_aos.data, 32)

	for i = 0, stb_aos.MAP_X - 1 do
		local greenish = {4, 2, 0, 0, 0, 0, 2, 4, 7, 8, 9, 9, 9, 9, 8, 6, 4}
		local g = math.floor((i * 16) / stb_aos.MAP_X)
		local g0 = greenish[g + 1]
		local g1 = greenish[g + 2]
		local off = (i * 16) / stb_aos.MAP_X - g
		local t = stb_aos.stb_lerp(off, g0, g1) / 9.0
		local top = stb_aos.stb_lerp(t, 16 + 16 * green_terrain_bias, 16)
		local bottom = stb_aos.stb_lerp(t, -8 + 16 * green_terrain_bias, -8)

		for j = 0, stb_aos.MAP_Y - 1 do
			stb_aos.data[i][j] = (stb_aos.data[i][j] * stb_aos.data[i][j]) / 16.0
			stb_aos.data[i][j] = stb_aos.stb_linear_remap(stb_aos.data[i][j], 0, 24, bottom, top)
		end
	end

	stb_aos.compute_water_height(water_squares_desired)

	stb_aos.grid_random(stb_aos.temp, 64, 64, -1, 1, 0, 0, 0, stb_aos.stb_frand() * stb_aos.MAP_Z)
	for i = 0, stb_aos.MAP_X - 1 do
		for j = 0, stb_aos.MAP_Y - 1 do
			local w_raw = stb_aos.stb_linear_remap(stb_aos.data[i][j], 0, 4, 1, 0)
			local w = stb_aos.smoothstep(w_raw)
			w = stb_aos.stb_lerp(w, 0.25, 1)
			stb_aos.data[i][j] = stb_aos.data[i][j] + w * stb_aos.temp[i][j]
		end
	end

	for i = 0, stb_aos.MAP_X - 1 do
		for j = 0, stb_aos.MAP_Y - 1 do
			local p = math.abs(i - stb_aos.MAP_X / 2)
			local t = stb_aos.stb_linear_remap(p, 85 - 45, 170 - 62, 1, 0)
			t = stb_aos.smooth_cubestep(t)
			stb_aos.data[i][j] = stb_aos.data[i][j] + 64 * t
			if stb_aos.data[i][j] > 63.5 then
				stb_aos.data[i][j] = 63.5
			end
		end
	end

	stb_aos.build_height_array()
	stb_aos.add_lighting_detail()
	stb_aos.compute_ground_lighting(4, 24)
end

-- Function to create a pillar
local function make_pillar(x, y)
	local h = (stb_aos.stb_rand() % 40) + 10
	if x < 128 or x >= 384 then
		if stb_aos.stb_rand() % 10 < 7 then
			h = stb_aos.stb_rand() % 20 + 30
		end
	end

	pillar[num_pillars + 1] = {
		x = x,
		y = y,
		h = h,
		c = {0, 0, 0}
	}
	num_pillars = num_pillars + 1
end

local function connect_pillar(i, j)
	local rad = 2
	if (stb_aos.stb_rand() % 10 < 7) then
		rad = rad + 1
	end
	local bottom = -1
	if rad == 2 then
		bottom = -2
	end

	local x = math.floor((pillar[i].x + pillar[j].x) / 2 + stb_aos.stb_rand() % 15 - 7)
	local y = math.floor((pillar[i].y + pillar[j].y) / 2 + stb_aos.stb_rand() % 15 - 7)
	local z = math.floor((pillar[i].h + pillar[j].h) / 2)

	stb_aos.draw_partial_sphere_line(
		pillar[i].x,
		pillar[i].y,
		pillar[i].h,
		x,
		y,
		z,
		rad,
		bottom,
		2,
		pillar[i].c,
		pillar[j].c,
		0
	)
	stb_aos.draw_partial_sphere_line(
		x,
		y,
		z,
		pillar[j].x,
		pillar[j].y,
		pillar[j].h,
		rad,
		bottom,
		2,
		pillar[i].c,
		pillar[j].c,
		0
	)
end

local function make_skyways()
	for i = 1, num_pillars do
		local best_for_dir = {}
		local best_distance = {}
		local x = pillar[i].x
		local y = pillar[i].y
		local h = pillar[i].h

		for j = 1, NUM_PASSAGES do
			best_for_dir[j] = -1
			best_distance[j] = 160 * 160
		end

		for j = 1, num_pillars do
			if i ~= j then
				local dx = pillar[j].x - x
				local dy = pillar[j].y - y
				local dz = math.abs(pillar[j].h - h)
				local d = dx * dx + dy * dy + dz * dz * dz

				for k = 1, NUM_PASSAGES do
					if d < best_distance[k] then
						table.insert(best_for_dir, k, j)
						table.insert(best_distance, k, d)
						table.remove(best_for_dir, NUM_PASSAGES + 1)
						table.remove(best_distance, NUM_PASSAGES + 1)
						break
					end
				end
			end
		end

		local n = NUM_PASSAGES - (stb_aos.stb_rand() % 4)
		for k = 1, n do
			if best_for_dir[k] >= 0 then
				local j = best_for_dir[k]
				if nearby_pillar[i][j] == 0 then
					connect_pillar(i, j)
					nearby_pillar[i][j] = 1
					nearby_pillar[j][i] = 1
				end
			end
		end
	end
end

local function draw_pillar(x, y, z)
	z = z + 1
	stb_aos.draw_rotated_ellipse(
		x,
		y,
		z + 2,
		stb_aos.stb_frand() * math.pi,
		stb_aos.stb_frand() * 4 + 3,
		stb_aos.stb_frand() * 4 + 3,
		stb_aos.stb_frand() * 3 + 2,
		0
	)
	if stb_aos.height[x][y] > z and x >= 128 - 32 and x <= 384 + 32 then
		stb_aos.draw_column(x, y, z - 2, z + 4, 0x80ffffff)
	end
end

function main(node, _, _)
	build_ground()

	-- now build the sky bridges
	--
	-- start by placing one pillar in every grid tile, jittered

	local tile = stb_aos.MAP_X / 8

	for i = tile, stb_aos.MAP_X - tile, tile do
		for j = tile, stb_aos.MAP_Y - tile, tile do
			local x = i + stb_aos.stb_rand() % (tile - 12) + 4
			local y = j + stb_aos.stb_rand() % (tile - 12) + 6
			make_pillar(x, y)
		end
	end

	-- now place an equal number more pillars, but never too close to an existing pillar

	for _ = 1, 200 do
		local i = stb_aos.stb_rand() % (stb_aos.MAP_X - 16) + 8
		local j = stb_aos.stb_rand() % (stb_aos.MAP_Y - 16) + 8
		local n = 0
		for k = 1, num_pillars do
			n = k
			if math.abs(pillar[k].x - i) < 22 and math.abs(pillar[k].y - j) < 14 then
				break
			end
		end
		if n == num_pillars then
			make_pillar(i, j)
		end
	end

	make_skyways()

	for n = 1, num_pillars do
		local i = pillar[n].x
		local j = pillar[n].y
		local k = pillar[n].h
		draw_pillar(i, j, k)
	end

	for i = 0, stb_aos.MAP_X - 1 do
		for j = 0, stb_aos.MAP_Y - 1 do
			local h = stb_aos.height[i][j] - 3
			for k = 0, h do
				if stb_aos.is_surface(i, j, k) then
					local n = stb_aos.perlin_noise3(i / 8.0, j / 8.0, k / 6.0, 1, 0, 0)
					local m = stb_aos.perlin_noise3(i / 13.0, j / 13.0, k / 12.0, 2, 0, 0)
					local d = stb_aos.perlin_noise3(i / 5.0, j / 5.0, k / 5.0, 3, 0, 0)
					local t = math.sin(k / 4.0 + d * 3) / 2 + 1

					local r0 = stb_aos.stb_lerp(n, 60, 100)
					local g0 = stb_aos.stb_lerp(n, 60, 70)
					local b0 = stb_aos.stb_lerp(n, 60, 50)

					local r1 = stb_aos.stb_lerp(m, 80, 100)
					local g1 = stb_aos.stb_lerp(m, 80, 120)
					local b1 = stb_aos.stb_lerp(m, 80, 130)

					local r = math.floor(stb_aos.stb_lerp(t, r0, r1))
					local g = math.floor(stb_aos.stb_lerp(t, g0, g1))
					local b = math.floor(stb_aos.stb_lerp(t, b0, b1))
					stb_aos.color[i][j][k] = 0x80000000 + (r << 16) + (g << 8) + b
				end
			end
		end
	end

	for i = 256 - 30, 256 + 30 do
		for j = 0, stb_aos.MAP_Y - 1 do
			local c
			if i % 2 == 0 then
				c = 0x80c0c0a0
			else
				c = 0x80c8b8a0
			end
			stb_aos.color[i][j][62] = c
		end
	end

	stb_aos.generate_voxels(node)
end
