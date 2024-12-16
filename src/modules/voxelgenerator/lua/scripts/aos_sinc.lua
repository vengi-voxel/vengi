local stb_aos = require "modules/stb_aos"

-- given a number from 0..1, make sure it wraps around smoothly
local function match_edges(x)
	if x < 1.0 / 16 then
		return stb_aos.stb_linear_remap(x, 0, 1.0 / 16, 0.5, 1.0 / 16)
	end
	if x >= 15.0 / 16 then
		return stb_aos.stb_linear_remap(x, 15.0 / 16, 1, 15.0 / 16, 0.5)
	end
	return x
end

local function build_ground()
	local low00, low01, low10, low11
	local high00, high01, high10, high11
	local lh00, lh01, lh10, lh11
	local hh00, hh01, hh10, hh11

	local rate = stb_aos.stb_lerp(stb_aos.stb_frand(), 28, 44)
	local spread = stb_aos.stb_lerp(stb_aos.stb_frand(), 0.6, 0.9)

	high00 = stb_aos.stb_lerp(stb_aos.stb_frand(), 11, 24)
	high01 = stb_aos.stb_lerp(stb_aos.stb_frand(), 9, 18)
	high10 = stb_aos.stb_lerp(stb_aos.stb_frand(), 11, 24)
	high11 = stb_aos.stb_lerp(stb_aos.stb_frand(), 9, 18)

	hh00 = stb_aos.stb_lerp(stb_aos.stb_frand(), 8, 16)
	hh01 = stb_aos.stb_lerp(stb_aos.stb_frand(), 8, 13)
	hh10 = stb_aos.stb_lerp(stb_aos.stb_frand(), 8, 16)
	hh11 = stb_aos.stb_lerp(stb_aos.stb_frand(), 8, 13)

	low00 = stb_aos.stb_lerp(stb_aos.stb_frand(), 2, 6)
	low01 = stb_aos.stb_lerp(stb_aos.stb_frand(), -1, 4)
	low10 = stb_aos.stb_lerp(stb_aos.stb_frand(), 2, 6)
	low11 = stb_aos.stb_lerp(stb_aos.stb_frand(), -1, 4)

	lh00 = stb_aos.stb_lerp(stb_aos.stb_frand(), 7, 11)
	lh01 = stb_aos.stb_lerp(stb_aos.stb_frand(), 5, 11)
	lh10 = stb_aos.stb_lerp(stb_aos.stb_frand(), 7, 11)
	lh11 = stb_aos.stb_lerp(stb_aos.stb_frand(), 5, 11)

	for i = 0, stb_aos.MAP_X - 1 do
		local u = match_edges(stb_aos.stb_linear_remap(i, 0, stb_aos.MAP_X, 0.0, 1.0))
		local h0 = stb_aos.stb_lerp(u, high00, high01)
		local h1 = stb_aos.stb_lerp(u, high10, high11)
		local l0 = stb_aos.stb_lerp(u, low00, low01)
		local l1 = stb_aos.stb_lerp(u, low10, low11)
		local dh0 = stb_aos.stb_lerp(u, hh00, hh01)
		local dh1 = stb_aos.stb_lerp(u, hh10, hh11)
		local dl0 = stb_aos.stb_lerp(u, lh00, lh01)
		local dl1 = stb_aos.stb_lerp(u, lh10, lh11)
		for j = 0, stb_aos.MAP_Y - 1 do
			local v = match_edges(stb_aos.stb_linear_remap(j, 0, stb_aos.MAP_Y, 0.0, 1.0))
			local fh = stb_aos.stb_lerp(v, h0, h1)
			local fl = stb_aos.stb_lerp(v, l0, l1)
			local fdh = stb_aos.stb_lerp(v, dh0, dh1)
			local fdl = stb_aos.stb_lerp(v, dl0, dl1)
			local bi = (i + stb_aos.MAP_X / 2) & stb_aos.MASK_X
			local bj = (j + stb_aos.MAP_Y / 2) & stb_aos.MASK_Y
			local x = (bi - (stb_aos.MAP_X / 2 - 0.5)) / (stb_aos.MAP_X / 2)
			local y = (bj - (stb_aos.MAP_X / 2 - 0.5)) / (stb_aos.MAP_X / 2)
			local r = math.sqrt(x * x + y * y) -- 0 .. 1.4
			-- local bottom = stb_aos.stb_lerp(r, 16, 2)
			-- local top = stb_aos.stb_lerp(r, 26, 10)
			local bottom = stb_aos.stb_lerp(r, fh, fl)
			local top = stb_aos.stb_lerp(r, fh + fdh, fl + fdl)
			local ang = r ^ spread * rate
			local h = stb_aos.stb_lerp(math.cos(ang) / 2 + 0.5, bottom, top)
			stb_aos.data[i][j] = h
		end
	end

	-- now add some randomness to the shores

	stb_aos.add_shore_detail()

	for i = 0, stb_aos.MAP_X - 1 do
		for j = 0, stb_aos.MAP_Y - 1 do
			local z = math.floor(stb_aos.data[i][j])
			if z < 0 then
				z = 0
			end
			stb_aos.height[i][j] = z
			for k = 0, stb_aos.MAP_Z - 1 do
				stb_aos.map[i][j][k] = (k <= z)
			end
		end
	end

	-- add some lighting detail
	stb_aos.add_lighting_detail()

	stb_aos.compute_ground_lighting(4, 24)
end

function main(node, _, _)
	build_ground()

	stb_aos.generate_voxels(node)
end
