local stb_aos = require "modules/stb_aos"

local function build_ground()
	local x, y
	local rx, ry

	-- Generate the "forest" data into 'data'
	stb_aos.grid_random(
		stb_aos.temp,
		2,
		2,
		0,
		20,
		0,
		math.random() * stb_aos.MAP_X,
		math.random() * stb_aos.MAP_Y,
		math.random() * stb_aos.MAP_Z
	)
	stb_aos.grid_random(
		stb_aos.data,
		4,
		4,
		0,
		20,
		0,
		math.random() * stb_aos.MAP_X,
		math.random() * stb_aos.MAP_Y,
		math.random() * stb_aos.MAP_Z
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		8,
		8,
		0,
		16,
		0,
		math.random() * stb_aos.MAP_X,
		math.random() * stb_aos.MAP_Y,
		math.random() * stb_aos.MAP_Z
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)
	stb_aos.grid_random(
		stb_aos.data,
		16,
		16,
		0,
		14,
		0,
		math.random() * stb_aos.MAP_X,
		math.random() * stb_aos.MAP_Y,
		math.random() * stb_aos.MAP_Z
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.data, 1)

	-- Copy 'temp' into 'data'
	for i = 0, stb_aos.MAP_X - 1 do
		for j = 0, stb_aos.MAP_Y - 1 do
			stb_aos.data[i][j] = stb_aos.temp[i][j]
		end
	end

	-- Generate the "river" terrain into 'temp'
	stb_aos.grid_random(
		stb_aos.temp,
		8,
		8,
		0,
		10,
		0,
		math.random() * stb_aos.MAP_X,
		math.random() * stb_aos.MAP_Y,
		math.random() * stb_aos.MAP_Z
	)
	stb_aos.grid_random(
		stb_aos.temp2,
		16,
		16,
		0,
		10,
		0,
		math.random() * stb_aos.MAP_X,
		math.random() * stb_aos.MAP_Y,
		math.random() * stb_aos.MAP_Z
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.temp2, 1)
	stb_aos.grid_random(
		stb_aos.temp2,
		32,
		32,
		0,
		8,
		0,
		math.random() * stb_aos.MAP_X,
		math.random() * stb_aos.MAP_Y,
		math.random() * stb_aos.MAP_Z
	)
	stb_aos.grid_addeq_scale(stb_aos.temp, stb_aos.temp2, 1)

	stb_aos.grid_minimize(stb_aos.data)
	stb_aos.grid_minimize(stb_aos.temp)

	-- Compute weighting for green vs. blue terrain
	local greenish = {2, 1, 0, 0, 0, 0, 2, 4, 5, 7, 9, 9, 9, 9, 8, 7, 2}
	for i = 0, stb_aos.MAP_X - 1 do
		local g = math.floor((i * 16) / stb_aos.MAP_X) + 1
		local g0 = greenish[g]
		local g1 = greenish[g + 1]
		local off = (i * 16) / stb_aos.MAP_X - (g - 1)
		local t = stb_aos.stb_lerp(off, g0, g1) / 9.0

		for j = 0, stb_aos.MAP_Y - 1 do
			stb_aos.data[i][j] = stb_aos.stb_lerp(t, stb_aos.data[i][j], stb_aos.temp[i][j]) + 1.5
		end
	end

	s = math.random() * 64

	-- Choose "fork" location for river
	x = 384 + math.random(-64, 63)
	y = 256 + math.random(-75, 74)

	-- Draw "ocean"
	for i = 0, 30 do
		local ty = math.floor(256 + (y - 256) / 2)
		local range = math.abs(i - 15)
		range = math.sqrt(15 * 15 - range * range)
		range = math.floor(range * 12)
		for j = -range, range do
			stb_aos.water[stb_aos.MAP_X - 1 - i][(ty + j) % stb_aos.MAP_Y] = 1
		end
	end

	rx = x
	ry = y

	-- Draw river delta
	for i = 0, 8, 2 do
		stb_aos.draw_random_river(x, y, 4, i - 4, 2 + math.random(0, 2))
	end

	-- Draw river into blue territory
	local last_vertical = 0
	while x > 0 do
		local dx = -24 + math.random(0, 11)
		local dy

		if x > stb_aos.MAP_X / 2 then
			dy = math.random(-12, 11)
		else
			dy = math.random(-20, 19)
		end

		if y < stb_aos.MAP_Y / 8 then
			dy = dy + 20
		elseif y < stb_aos.MAP_Y / 4 then
			dy = dy + 10
		elseif y >= stb_aos.MAP_Y - stb_aos.MAP_Y / 8 then
			dy = dy - 20
		elseif y >= stb_aos.MAP_Y - stb_aos.MAP_Y / 4 then
			dy = dy - 10
		end

		if math.abs(dy) > 12 and last_vertical == 0 and x < stb_aos.MAP_X / 2 + stb_aos.MAP_X / 8 then
			last_vertical = 1
			dx = math.floor(dx / 2)
		else
			last_vertical = 0
		end

		if x + dx < 0 then
			dx = -x
		end

		stb_aos.draw_river_segment(x, y, x + dx, y + dy, 3)
		x = x + dx
		y = y + dy
	end
	stb_aos.draw_river_segment(stb_aos.MAP_X - 1, y, rx, ry, 2)

	-- Create riverbed to follow water
	stb_aos.make_water()

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

	stb_aos.compute_ground_lighting(4, 24)
end

function description()
	return "Generates a forest with a river."
end

function main(node, _, _)
	build_ground()
	stb_aos.place_trees(-0.5, 0.75)

	stb_aos.generate_voxels(node)
end
