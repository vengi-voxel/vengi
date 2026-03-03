function description()
	return "Colosseum - a detailed brick-by-brick voxel recreation of the Roman amphitheater in style"
end

function arguments()
	return {
		{name = "seed", desc = "Random seed for brick color variation", type = "int", default = "42"},
		{name = "arches", desc = "Number of arches per level (32-64)", type = "int", default = "48"},
		{name = "damaged", desc = "Show south-wall damage", type = "bool", default = "0"}
	}
end

function main(node, region, color, seed, arches, damaged)
	-- ============================================================
	-- Constants & dimensions
	-- ============================================================
	local PI = math.pi
	local TWO_PI = 2 * PI
	local floor = math.floor
	local sqrt = math.sqrt
	local sin = math.sin
	local cos = math.cos
	local atan2 = math.atan
	local abs = math.abs
	local min = math.min
	local max = math.max

	-- Volume dimensions
	local W = 256
	local D = 256
	local H = 100

	-- Center of the ellipse in XZ
	local CX = 128
	local CZ = 128

	-- Ellipse semi-axes (outer wall)
	local OUTER_A = 95 -- long axis (x)
	local OUTER_B = 78 -- short axis (z)
	local WALL_THICK = 7
	local INNER_A = OUTER_A - WALL_THICK -- 88
	local INNER_B = OUTER_B - WALL_THICK -- 71

	-- Arena ellipse (playing surface)
	local ARENA_A = 42
	local ARENA_B = 26

	-- Podium wall (surrounds arena)
	local PODIUM_A = ARENA_A + 2
	local PODIUM_B = ARENA_B + 2

	-- Number of arches per story
	local NUM_ARCHES = arches

	-- Story dimensions
	local BASE_Y = 4 -- ground level (below = foundation)
	local STORY_H = 19 -- voxels per story
	local NUM_STORIES = 4
	local TOTAL_H = STORY_H * NUM_STORIES -- 76

	-- Arch opening geometry within each story (story-local y)
	local ARCH_FLOOR = 2 -- bottom of arch opening
	local ARCH_RECT_TOP = 13 -- top of rectangular part
	local ARCH_ROUND_R = 3 -- radius of semicircular top
	local ARCH_MARGIN = 0.27 -- fraction of segment that is pillar on each side (arch opening = 1 - 2*margin)

	-- Seating
	local SEAT_OUTER_A = INNER_A - 1 -- 87
	local SEAT_OUTER_B = INNER_B - 1 -- 70
	local NUM_TIERS = 28
	local TIER_H = 2

	-- ============================================================
	-- Create scene node
	-- ============================================================
	local mapRegion = g_region.new(0, 0, 0, W - 1, H - 1, D - 1)
	local mapNode = g_scenegraph.new("Colosseum", mapRegion)
	local vol = mapNode:volume()
	local pal = mapNode:palette()

	local function addColor(r, g, b)
		local _, idx = pal:tryAdd(r, g, b, 255, false)
		return idx
	end

	local COL_TAN = addColor(228, 205, 158)
	local COL_TAN_DARK = addColor(199, 178, 137)
	local COL_TAN_LIGHT = addColor(242, 224, 181)
	local COL_BRICK_TAN = addColor(214, 192, 148)
	local COL_MORTAR = addColor(185, 165, 125)
	local COL_DARK_TAN = addColor(180, 155, 110)
	local COL_DARK_TAN2 = addColor(168, 143, 100)
	local COL_COLUMN = addColor(238, 225, 195)
	local COL_COLUMN_DARK = addColor(210, 195, 160)
	local COL_COLUMN_CAP = addColor(245, 235, 210)
	local COL_CORNICE = addColor(240, 228, 195)
	local COL_CORNICE2 = addColor(248, 238, 210)
	local COL_SEAT_LIGHT = addColor(180, 180, 180)
	local COL_SEAT_MED = addColor(156, 156, 156)
	-- local COL_SEAT_DARK = addColor(120, 120, 120)
	local COL_SEAT_STRUCT = addColor(100, 100, 100)
	local COL_SAND = addColor(222, 200, 148)
	local COL_SAND_DARK = addColor(195, 175, 128)
	local COL_FOUNDATION = addColor(155, 135, 100)
	local COL_UNDERGROUND = addColor(85, 65, 40)
	local COL_GREEN = addColor(88, 140, 56)
	local COL_GREEN_DARK = addColor(70, 112, 44)
	-- local COL_STUD_TAN = addColor(248, 230, 185)
	-- local COL_STUD_GRAY = addColor(200, 200, 200)
	-- local COL_STUD_GREEN = addColor(105, 160, 72)
	-- local COL_STUD_SAND = addColor(238, 218, 168)
	local COL_REDDISH_BRN = addColor(124, 80, 42)
	local COL_PODIUM = addColor(192, 172, 138)
	local COL_BRACKET = addColor(175, 150, 108)

	g_log.info("Palette ready-building Colosseum")

	-- ============================================================
	-- Deterministic RNG (xorshift32) for brick variation
	-- ============================================================
	local rng_state = seed
	if rng_state == 0 then
		rng_state = 1
	end

	local function rng()
		local s = rng_state
		s = ((s ~ (s << 13)) & 0xFFFFFFFF)
		s = ((s ~ (s >> 17)) & 0xFFFFFFFF)
		s = ((s ~ (s << 5)) & 0xFFFFFFFF)
		rng_state = s
		return (s & 0x7FFFFFFF) / 0x7FFFFFFF
	end

	--- Normalised elliptical distance from centre (1.0 = on the ellipse).
	local function ellipDist(x, z, a, b)
		local dx = (x - CX) / a
		local dz = (z - CZ) / b
		return sqrt(dx * dx + dz * dz)
	end

	--- Radial distance from centre on the ellipse at a given angle.
	local function ellipRadius(a, b, ca, sa)
		return (a * b) / sqrt((b * ca) * (b * ca) + (a * sa) * (a * sa))
	end

	--- brick colour with running-bond pattern.
	local function brickColor(x, y, z, story)
		local brick_row = floor(y / 3)
		local y_in_brick = y % 3

		if y_in_brick == 2 then
			return COL_MORTAR -- horizontal mortar line
		end

		-- Running-bond vertical joints every 4 voxels, offset every other row
		local offset = (brick_row % 2 == 0) and 0 or 2
		local bx = floor((x + offset) / 4)
		local bz = floor(z / 4)

		-- Deterministic per-brick colour variant
		local h = ((bx * 73856093) ~ (bz * 19349663) ~ (brick_row * 83492791)) & 0xFFFF
		local v = h % 4
		if v == 0 then
			return COL_TAN
		end
		if v == 1 then
			return COL_BRICK_TAN
		end
		if v == 2 then
			return COL_TAN_LIGHT
		end
		return COL_TAN_DARK
	end

	local segment_angle = TWO_PI / NUM_ARCHES

	-- ============================================================
	-- PHASE 1 - Foundation & green baseplate
	-- ============================================================
	g_log.info("Phase 1/9: Foundation & baseplate")

	for x = CX - OUTER_A - 15, CX + OUTER_A + 15 do
		for z = CZ - OUTER_B - 15, CZ + OUTER_B + 15 do
			local r = ellipDist(x, z, OUTER_A + 12, OUTER_B + 12)
			if r <= 1.0 then
				local r2 = ellipDist(x, z, OUTER_A + 2, OUTER_B + 2)
				for y = 0, BASE_Y - 1 do
					if r2 <= 1.0 then
						vol:setVoxel(x, y, z, COL_FOUNDATION) -- stone foundation
					else
						vol:setVoxel(x, y, z, (y == BASE_Y - 1) and COL_GREEN or COL_GREEN_DARK)
					end
				end
			end
		end
	end

	-- ============================================================
	-- PHASE 2 - Outer wall with arches (main structure)
	-- ============================================================
	g_log.info("Phase 2/9: Outer wall & arches")

	for x = CX - OUTER_A - 1, CX + OUTER_A + 1 do
		for z = CZ - OUTER_B - 1, CZ + OUTER_B + 1 do
			local r_outer = ellipDist(x, z, OUTER_A, OUTER_B)
			local r_inner = ellipDist(x, z, INNER_A, INNER_B)

			if r_outer <= 1.0 and r_inner >= 1.0 then
				-- Angle for arch-segment lookup
				local theta = atan2(z - CZ, x - CX)
				local theta_pos = theta + PI -- 0 ... 2π
				local seg_idx = floor(theta_pos / segment_angle)
				local frac = (theta_pos - seg_idx * segment_angle) / segment_angle

				-- Wall-depth fraction (0 = inner face, 1 = outer face)
				local ca = cos(theta)
				local sa = sin(theta)
				local dist = sqrt((x - CX) * (x - CX) + (z - CZ) * (z - CZ))
				local r_inn = ellipRadius(INNER_A, INNER_B, ca, sa)
				local r_out = ellipRadius(OUTER_A, OUTER_B, ca, sa)
				local wall_frac = (dist - r_inn) / (r_out - r_inn)

				for y = BASE_Y, BASE_Y + TOTAL_H - 1 do
					local y_rel = y - BASE_Y
					local story = floor(y_rel / STORY_H)
					local y_in_st = y_rel % STORY_H

					-- ---- Determine whether this voxel is an arch opening ----
					local is_opening = false

					if story < 3 then
						-- Stories 0-2: semicircular arches
						local in_arch_zone = frac > ARCH_MARGIN and frac < (1.0 - ARCH_MARGIN)

						if in_arch_zone then
							local depth_ok = wall_frac > 0.35 -- opening cuts through outer 65 %
							if depth_ok then
								if y_in_st >= ARCH_FLOOR and y_in_st <= ARCH_RECT_TOP then
									is_opening = true
								elseif y_in_st > ARCH_RECT_TOP and y_in_st <= ARCH_RECT_TOP + ARCH_ROUND_R then
									-- Semicircular top
									local half_span = (1.0 - 2 * ARCH_MARGIN) / 2.0
									local nx2 = (frac - 0.5) / half_span -- -1 ... 1
									local ny2 = (y_in_st - ARCH_RECT_TOP) / ARCH_ROUND_R -- 0 ... 1
									if nx2 * nx2 + ny2 * ny2 < 1.0 then
										is_opening = true
									end
								end
							end
						end
					elseif story == 3 then
						-- Attic: small rectangular windows, every other segment
						if seg_idx % 2 == 0 then
							local in_win = frac > 0.32 and frac < 0.68
							if in_win and y_in_st >= 4 and y_in_st <= 12 and wall_frac > 0.55 then
								is_opening = true
							end
						end
					end

					-- ---- Optional damage on south side ----
					if damaged and story >= 2 then
						local angle_deg = theta_pos * 180 / PI
						if angle_deg > 130 and angle_deg < 230 then
							local dmg_center = 180
							local dmg_spread = 45
							local dmg_factor = 1.0 - abs(angle_deg - dmg_center) / dmg_spread
							if dmg_factor > 0 then
								local max_y_rel = TOTAL_H * (1.0 - dmg_factor * 0.55)
								if y_rel > max_y_rel + rng() * 6 then
									is_opening = true
								end
							end
						end
					end

					-- ---- Place voxel ----
					if not is_opening then
						local c
						if y_in_st == STORY_H - 1 then
							c = COL_CORNICE -- cornice band at top of each story
						elseif y_in_st == STORY_H - 2 then
							c = COL_CORNICE2
						elseif y_in_st == 0 then
							c = COL_DARK_TAN -- floor band
						else
							c = brickColor(x, y_rel, z, story)
						end
						vol:setVoxel(x, y, z, c)
					end
				end
			end
		end
	end

	-- ============================================================
	-- PHASE 3 - Engaged columns on the outer face
	-- ============================================================
	g_log.info("Phase 3/9: Columns & pilasters")

	for i = 0, NUM_ARCHES - 1 do
		local angle = -PI + i * segment_angle
		local ca = cos(angle)
		local sa = sin(angle)

		-- Point on outer ellipse
		local r_at = ellipRadius(OUTER_A, OUTER_B, ca, sa)
		local px = CX + r_at * ca
		local pz = CZ + r_at * sa

		-- Outward normal at this point on the ellipse
		local nx = ca / (OUTER_A * OUTER_A)
		local nz = sa / (OUTER_B * OUTER_B)
		local nl = sqrt(nx * nx + nz * nz)
		nx = nx / nl
		nz = nz / nl

		-- Tangent (perpendicular to normal in XZ)
		local tx = -nz
		local tz = nx

		-- Stories 0-2: half-columns
		for story = 0, 2 do
			local y0 = BASE_Y + story * STORY_H + 1
			local y1 = BASE_Y + (story + 1) * STORY_H - 3

			for y = y0, y1 do
				local is_base = (y - y0) < 2
				local is_capital = (y1 - y) < 2
				local wRange = is_base and 1 or (is_capital and 1 or 0)

				for w = -wRange, wRange do
					for d = 0, 1 do
						local xx = floor(px + nx * d + tx * w + 0.5)
						local zz = floor(pz + nz * d + tz * w + 0.5)
						if ellipDist(xx, zz, OUTER_A, OUTER_B) >= 0.99 then
							local c = (is_base or is_capital) and COL_COLUMN_CAP or COL_COLUMN
							vol:setVoxel(xx, y, zz, c)
						end
					end
				end
			end
		end

		-- Story 3: flat pilasters
		local y0 = BASE_Y + 3 * STORY_H + 1
		local y1 = BASE_Y + 4 * STORY_H - 3
		for y = y0, y1 do
			for w = -1, 1 do
				local xx = floor(px + tx * w + 0.5)
				local zz = floor(pz + tz * w + 0.5)
				if ellipDist(xx, zz, OUTER_A, OUTER_B) >= 0.99 then
					vol:setVoxel(xx, y, zz, COL_COLUMN_DARK)
				end
			end
		end
	end

	-- ============================================================
	-- PHASE 4 - Cornices (protruding ledges)
	-- ============================================================
	g_log.info("Phase 4/9: Cornices")

	for story = 0, NUM_STORIES - 1 do
		local cy1 = BASE_Y + (story + 1) * STORY_H - 1
		local cy2 = cy1 - 1

		for x = CX - OUTER_A - 3, CX + OUTER_A + 3 do
			for z = CZ - OUTER_B - 3, CZ + OUTER_B + 3 do
				local r = ellipDist(x, z, OUTER_A + 1, OUTER_B + 1)
				local r2 = ellipDist(x, z, OUTER_A, OUTER_B)
				if r <= 1.0 and r2 > 1.0 then
					-- Check damage
					local skip = false
					if damaged and story >= 2 then
						local theta_pos = atan2(z - CZ, x - CX) + PI
						local angle_deg = theta_pos * 180 / PI
						if angle_deg > 130 and angle_deg < 230 then
							local f = 1.0 - abs(angle_deg - 180) / 45
							if f > 0 and (story * STORY_H) > TOTAL_H * (1.0 - f * 0.55) then
								skip = true
							end
						end
					end
					if not skip then
						vol:setVoxel(x, cy1, z, COL_CORNICE)
						vol:setVoxel(x, cy2, z, COL_CORNICE2)
					end
				end
			end
		end
	end

	-- Top parapet (capping cornice + 1 voxel)
	local top_y = BASE_Y + TOTAL_H
	for x = CX - OUTER_A - 3, CX + OUTER_A + 3 do
		for z = CZ - OUTER_B - 3, CZ + OUTER_B + 3 do
			local r = ellipDist(x, z, OUTER_A + 2, OUTER_B + 2)
			local r2 = ellipDist(x, z, OUTER_A + 1, OUTER_B + 1)
			if r <= 1.0 and r2 > 1.0 then
				-- damage check
				local skip = false
				if damaged then
					local theta_pos = atan2(z - CZ, x - CX) + PI
					local angle_deg = theta_pos * 180 / PI
					if angle_deg > 130 and angle_deg < 230 then
						local f = 1.0 - abs(angle_deg - 180) / 45
						if f > 0.15 then
							skip = true
						end
					end
				end
				if not skip then
					vol:setVoxel(x, top_y, z, COL_CORNICE)
					vol:setVoxel(x, top_y + 1, z, COL_CORNICE2)
				end
			end
		end
	end

	-- ============================================================
	-- PHASE 5 - Seating tiers (cavea)
	-- ============================================================
	g_log.info("Phase 5/9: Seating tiers")

	for x = CX - SEAT_OUTER_A - 1, CX + SEAT_OUTER_A + 1 do
		for z = CZ - SEAT_OUTER_B - 1, CZ + SEAT_OUTER_B + 1 do
			local r_out = ellipDist(x, z, SEAT_OUTER_A, SEAT_OUTER_B)
			local r_in = ellipDist(x, z, PODIUM_A, PODIUM_B)

			if r_out <= 1.0 and r_in >= 1.0 then
				local theta = atan2(z - CZ, x - CX)
				local ca = cos(theta)
				local sa = sin(theta)
				local dist = sqrt((x - CX) * (x - CX) + (z - CZ) * (z - CZ))

				local r_edge_in = ellipRadius(PODIUM_A, PODIUM_B, ca, sa)
				local r_edge_out = ellipRadius(SEAT_OUTER_A, SEAT_OUTER_B, ca, sa)

				local frac = (dist - r_edge_in) / (r_edge_out - r_edge_in)
				frac = max(0, min(1, frac))

				local tier = floor(frac * NUM_TIERS)
				if tier >= NUM_TIERS then
					tier = NUM_TIERS - 1
				end

				local tier_top_y = BASE_Y + 4 + tier * TIER_H

				for y = BASE_Y, tier_top_y do
					local c
					if y == tier_top_y then
						c = (tier % 2 == 0) and COL_SEAT_LIGHT or COL_SEAT_MED
					elseif y == tier_top_y - 1 and tier_top_y > BASE_Y + 1 then
						c = COL_SEAT_MED
					else
						c = COL_SEAT_STRUCT
					end
					vol:setVoxel(x, y, z, c)
				end

				-- Cut vomitoria (8 radial entrance passages)
				local theta_pos = theta + PI
				local vom_seg = floor(theta_pos / (TWO_PI / 8))
				local vom_frac = (theta_pos - vom_seg * (TWO_PI / 8)) / (TWO_PI / 8)
				if vom_frac > 0.46 and vom_frac < 0.54 then
					for y = BASE_Y, min(BASE_Y + 8, tier_top_y) do
						vol:setVoxel(x, y, z, -1)
					end
				end
			end
		end
	end

	-- ============================================================
	-- PHASE 6 - Arena floor & hypogeum
	-- ============================================================
	g_log.info("Phase 6/9: Arena floor & hypogeum")

	for x = CX - PODIUM_A, CX + PODIUM_A do
		for z = CZ - PODIUM_B, CZ + PODIUM_B do
			local r = ellipDist(x, z, PODIUM_A, PODIUM_B)
			if r <= 1.0 then
				local r_arena = ellipDist(x, z, ARENA_A, ARENA_B)
				local cx_rel = x - CX
				local cz_rel = z - CZ

				if r_arena > 1.0 and r <= 1.0 then
					-- Podium wall (arena enclosure)
					for y = BASE_Y, BASE_Y + 7 do
						vol:setVoxel(x, y, z, COL_PODIUM)
					end
				else
					-- Arena floor
					vol:setVoxel(x, BASE_Y + 1, z, COL_SAND)
					vol:setVoxel(x, BASE_Y, z, COL_SAND_DARK)

					-- Hypogeum pattern visible through the floor
					-- Central longitudinal spine wall
					if abs(cz_rel) <= 2 then
						vol:setVoxel(x, BASE_Y, z, COL_UNDERGROUND)
					end
					-- Transverse walls
					if abs(cx_rel) <= 1 then
						vol:setVoxel(x, BASE_Y, z, COL_UNDERGROUND)
					end
					-- Grid of hypogeum cells (cross-hatched pattern)
					if (abs(cx_rel) % 8 < 1) or (abs(cz_rel) % 8 < 1) then
						vol:setVoxel(x, BASE_Y, z, COL_UNDERGROUND)
					end
				end
			end
		end
	end

	-- ============================================================
	-- PHASE 7 - Radial buttress walls inside the structure
	-- ============================================================
	g_log.info("Phase 7/9: Radial buttress walls")

	for i = 0, NUM_ARCHES - 1 do
		local angle = -PI + i * segment_angle
		local ca = cos(angle)
		local sa = sin(angle)

		-- Walk radially from podium to inner wall face
		local r_start = ellipRadius(PODIUM_A + 1, PODIUM_B + 1, ca, sa)
		local r_end = ellipRadius(INNER_A, INNER_B, ca, sa)

		local steps = floor(r_end - r_start)
		if steps < 1 then
			steps = 1
		end

		for s = 0, steps do
			local t = s / steps
			local rd = r_start + t * (r_end - r_start)
			local wx = floor(CX + rd * ca + 0.5)
			local wz = floor(CZ + rd * sa + 0.5)

			-- Only within the gap between seating edge and inner wall
			local rd_check = ellipDist(wx, wz, SEAT_OUTER_A, SEAT_OUTER_B)
			if rd_check <= 1.0 then
				-- Buttress wall rises with the seating
				local max_wall_y = BASE_Y + min(TOTAL_H - 15, floor(4 + t * NUM_TIERS * TIER_H) + 10)
				for y = BASE_Y, max_wall_y do
					if vol:voxel(wx, y, wz) == -1 then
						vol:setVoxel(wx, y, wz, COL_DARK_TAN2)
					end
				end
			end
		end
	end

	-- ============================================================
	-- PHASE 8 - Mast corbels & poles on the top
	-- ============================================================
	g_log.info("Phase 8/9: Mast corbels & velarium poles")

	local NUM_MASTS = 80
	local mast_step = TWO_PI / NUM_MASTS

	for i = 0, NUM_MASTS - 1 do
		local angle = -PI + i * mast_step
		local ca = cos(angle)
		local sa = sin(angle)

		local r_at = ellipRadius(OUTER_A, OUTER_B, ca, sa)
		local bx = floor(CX + r_at * ca + 0.5)
		local bz = floor(CZ + r_at * sa + 0.5)

		-- Outward normal
		local nx = ca / (OUTER_A * OUTER_A)
		local nz = sa / (OUTER_B * OUTER_B)
		local nl = sqrt(nx * nx + nz * nz)
		nx = nx / nl
		nz = nz / nl

		local corbel_y = BASE_Y + TOTAL_H

		-- Damage skip
		local skip = false
		if damaged then
			local theta_pos = angle + PI
			local angle_deg = theta_pos * 180 / PI
			if angle_deg > 130 and angle_deg < 230 then
				local f = 1.0 - abs(angle_deg - 180) / 45
				if f > 0.15 then
					skip = true
				end
			end
		end

		if not skip then
			-- Bracket (3 voxels tall, 2 voxels deep)
			for dy = 0, 2 do
				for d = 0, 1 do
					local xx = floor(bx + nx * d + 0.5)
					local zz = floor(bz + nz * d + 0.5)
					vol:setVoxel(xx, corbel_y + dy, zz, COL_BRACKET)
				end
			end
			-- Mast pole
			for dy = 3, 8 do
				vol:setVoxel(bx, corbel_y + dy, bz, COL_REDDISH_BRN)
			end
		end
	end
end
