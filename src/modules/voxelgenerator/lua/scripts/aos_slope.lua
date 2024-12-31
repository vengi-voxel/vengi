local stb_aos = require "modules/stb_aos"

local function build_ground()
	local water_squares_desired =
		stb_aos.stb_lerp(stb_aos.stb_frand() ^ 1.5, 1.0 / 64, 1.0 / 16) * (stb_aos.MAP_X * stb_aos.MAP_Y)

	stb_aos.grid_random(
		stb_aos.temp,
		4,
		4,
		-4,
		4,
		0,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_random(
		stb_aos.data,
		2,
		2,
		-4,
		4,
		0,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		4,
		4,
		-8,
		8,
		1,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		8,
		8,
		-10,
		10,
		1,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		16,
		16,
		-6,
		6,
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
		-1.5,
		1.5,
		1,
		stb_aos.MAP_X * stb_aos.stb_frand(),
		stb_aos.MAP_Y * stb_aos.stb_frand(),
		stb_aos.MAP_Z * stb_aos.stb_frand()
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)

	stb_aos.data = stb_aos.temp

	for i = 0, stb_aos.MAP_X - 1 do
		local off = i / stb_aos.MAP_X
		if off < 1.0 / 16 or off > 15.0 / 16 then
			if off < 1.0 / 16 then
				off = stb_aos.stb_linear_remap(off, 0, 1.0 / 16, 0.5, 0)
			else
				off = stb_aos.stb_linear_remap(off, 15.0 / 16, 1.0, 1, 0.5)
			end
			off = stb_aos.smoothstep(off)
		else
			off = stb_aos.stb_linear_remap(off, 1.0 / 16, 15.0 / 16, 0, 1)
		end

		weight = math.abs(i - stb_aos.MAP_X / 2)
		weight = stb_aos.stb_linear_remap(weight, 0, stb_aos.MAP_X / 2, 1, 0.75)

		local t = off
		for j = 0, stb_aos.MAP_Y - 1 do
			stb_aos.data[i][j] = stb_aos.data[i][j] * weight + (1 - t) * 36 - 4
		end
	end

	stb_aos.grid_limit(stb_aos.data, 45)

	stb_aos.compute_water_height(water_squares_desired)

	stb_aos.grid_limit(stb_aos.data, 45)

	stb_aos.add_shore_detail()

	for i = 0, stb_aos.MAP_X - 1 do
		for j = 0, stb_aos.MAP_Y - 1 do
			local w_raw = stb_aos.stb_linear_remap(stb_aos.data[i][j], 0, 4, 1, 0)
			local w = stb_aos.smoothstep(w_raw)
			w = stb_aos.stb_lerp(w, 0.5, 1)
			stb_aos.data[i][j] = stb_aos.data[i][j] + w * stb_aos.temp[i][j]
		end
	end

	stb_aos.build_height_array()

	stb_aos.add_lighting_detail()

	stb_aos.compute_ground_lighting(4, 48)
end

function main(node, _, _)
	build_ground()

	stb_aos.generate_voxels(node)
end
