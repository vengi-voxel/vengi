local stb_aos = require "modules/stb_aos"

local function build_ground()
	stb_aos.grid_random(
		stb_aos.temp,
		2,
		2,
		0,
		6,
		0,
		256 * stb_aos.stb_frand(),
		256 * stb_aos.stb_frand(),
		64 * stb_aos.stb_frand()
	)
	stb_aos.grid_random(
		stb_aos.data,
		4,
		4,
		0,
		16,
		0,
		256 * stb_aos.stb_frand(),
		256 * stb_aos.stb_frand(),
		64 * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		8,
		8,
		0,
		20,
		0,
		256 * stb_aos.stb_frand(),
		256 * stb_aos.stb_frand(),
		64 * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		16,
		16,
		0,
		12,
		0,
		256 * stb_aos.stb_frand(),
		256 * stb_aos.stb_frand(),
		64 * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		32,
		32,
		0,
		8,
		0.5,
		256 * stb_aos.stb_frand(),
		256 * stb_aos.stb_frand(),
		64 * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		64,
		64,
		0,
		3,
		0,
		256 * stb_aos.stb_frand(),
		256 * stb_aos.stb_frand(),
		64 * stb_aos.stb_frand()
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

function main(node, _, _)
	build_ground()
	stb_aos.add_pillars(7)

	stb_aos.generate_voxels(node)
end
