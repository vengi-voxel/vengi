local stb_aos = require "modules/stb_aos"

local function build_ground()
	-- generate the blue terrain into stb_aos.data
	stb_aos.grid_random(
		stb_aos.temp,
		2,
		2,
		0,
		20,
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
		8,
		0,
		stb_aos.stb_frand() * stb_aos.MAP_X,
		stb_aos.stb_frand() * stb_aos.MAP_Y,
		stb_aos.stb_frand() * stb_aos.MAP_Z
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		8,
		8,
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
		6,
		0,
		stb_aos.stb_frand() * stb_aos.MAP_X,
		stb_aos.stb_frand() * stb_aos.MAP_Y,
		stb_aos.stb_frand() * stb_aos.MAP_Z
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.data = stb_aos.temp

	-- generate the green terrain into stb_aos.temp
	stb_aos.grid_random(
		stb_aos.temp,
		8,
		8,
		0,
		10,
		0,
		stb_aos.stb_frand() * stb_aos.MAP_X,
		stb_aos.stb_frand() * stb_aos.MAP_Y,
		stb_aos.stb_frand() * stb_aos.MAP_Z
	)
	stb_aos.grid_random(
		stb_aos.temp2,
		16,
		16,
		0,
		6,
		0,
		stb_aos.stb_frand() * stb_aos.MAP_X,
		stb_aos.stb_frand() * stb_aos.MAP_Y,
		stb_aos.stb_frand() * stb_aos.MAP_Z
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.temp2, 1)
	stb_aos.grid_random(
		stb_aos.temp2,
		32,
		32,
		0,
		4,
		0,
		stb_aos.stb_frand() * stb_aos.MAP_X,
		stb_aos.stb_frand() * stb_aos.MAP_Y,
		stb_aos.stb_frand() * stb_aos.MAP_Z
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.temp2, 1)

	stb_aos.grid_minimize(stb_aos.data)
	stb_aos.grid_minimize(stb_aos.temp)

	stb_aos.grid_limit(stb_aos.data, 32)

	for i = 0, stb_aos.MAP_X - 1 do
		local greenish = {2, 1, 0, 0, 0, 0, 2, 4, 5, 7, 9, 9, 9, 9, 8, 7, 2}
		local g = math.floor((i * 16) / stb_aos.MAP_X)
		local g0 = greenish[g + 1]
		local g1 = greenish[g + 2]
		local off = (i * 16) / stb_aos.MAP_X - g
		local t = stb_aos.stb_lerp(off, g0, g1) / 9.0

		for j = 0, stb_aos.MAP_Y - 1 do
			stb_aos.data[i][j] = stb_aos.stb_lerp(t, stb_aos.data[i][j], stb_aos.temp[i][j]) + 1.5
		end
	end

	-- build the moat
	for i = 384, stb_aos.MAP_X - 1 do
		for j = 128, 383 do
			stb_aos.water[i][j] = 1
		end
	end

	stb_aos.make_water()

	-- build the interior
	local MOAT_SIZE = 6
	for i = 384 + MOAT_SIZE, stb_aos.MAP_X - MOAT_SIZE - 1 do
		for j = 128 + MOAT_SIZE, 384 - MOAT_SIZE - 1 do
			local dx = (i < 448) and (i - (384 + MOAT_SIZE)) or (stb_aos.MAP_X - MOAT_SIZE - i)
			local dy = (j < 256) and (j - (128 + MOAT_SIZE)) or (384 - MOAT_SIZE - j)
			dx = math.abs(dx)
			dy = math.abs(dy)
			local d = math.min(dx, dy)
			d = stb_aos.stb_linear_remap(d, 0, 12, 1, 4)
			d = stb_aos.stb_clamp(d, 1, 3)
			stb_aos.data[i][j] = d
		end
	end

	-- add randomness to the shores
	stb_aos.grid_random(
		stb_aos.temp,
		16,
		16,
		-1,
		1,
		0,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_random(
		stb_aos.temp2,
		32,
		32,
		0,
		1,
		0,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.temp2, 1)
	stb_aos.grid_random(
		stb_aos.temp2,
		128,
		128,
		-1,
		0,
		0,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.temp2, 1)

	for i = 0, stb_aos.MAP_X - 1 do
		for j = 0, stb_aos.MAP_Y - 1 do
			local w_raw = stb_aos.stb_linear_remap(stb_aos.data[i][j], 0, 4, 1, 0)
			local w = stb_aos.smoothstep(w_raw)
			w = stb_aos.stb_lerp(w, 0, 3)
			stb_aos.data[i][j] = stb_aos.data[i][j] + w * stb_aos.temp[i][j] + stb_aos.temp[i][j] * 0.5
		end
	end

	stb_aos.build_height_array()
	stb_aos.add_lighting_detail()
	stb_aos.compute_ground_lighting(4, 24)
end

function description()
	return "Generates a fortress."
end

function main(node, _, _)
	build_ground()
	stb_aos.place_trees(-0.05, 0.7)

	local OUTER_WALL_HEIGHT = 5
	local MOAT_SIZE = 6
	local OUTER_WALL_COLOR = 0x80404040
	local STORY1_HEIGHT = 12
	local STORY2_HEIGHT = 28
	local CATWALK_COLOR = 0x80c0a060
	local BUILDING_COLOR = 0x80808080

	stb_aos.make_fortress(
		OUTER_WALL_HEIGHT,
		OUTER_WALL_COLOR,
		MOAT_SIZE,
		STORY1_HEIGHT,
		STORY2_HEIGHT,
		CATWALK_COLOR,
		BUILDING_COLOR
	)

	stb_aos.generate_voxels(node)
end
