--
-- Desert Rock Formation generator
-- Creates a cluster of tall columnar sandstone pillars/buttes with flat tops,
-- sitting on a sandy base with scattered small rocks and pebbles.
-- Inspired by desert mesa and Monument Valley-style formations.
--

function arguments()
	return {
		{ name = 'numPillars', desc = 'Number of rock pillars', type = 'int', default = '12', min = '4', max = '25' },
		{ name = 'maxHeight', desc = 'Maximum pillar height', type = 'int', default = '28', min = '10', max = '50' },
		{ name = 'minHeight', desc = 'Minimum pillar height', type = 'int', default = '8', min = '3', max = '25' },
		{ name = 'pillarWidth', desc = 'Base pillar width', type = 'int', default = '5', min = '2', max = '10' },
		{ name = 'clusterRadius', desc = 'How tightly pillars cluster together', type = 'int', default = '10', min = '4', max = '20' },
		{ name = 'baseRadius', desc = 'Sandy base radius', type = 'int', default = '18', min = '8', max = '30' },
		{ name = 'baseHeight', desc = 'Sandy base thickness', type = 'int', default = '2', min = '1', max = '5' },
		{ name = 'numPebbles', desc = 'Number of scattered pebbles/rocks', type = 'int', default = '10', min = '0', max = '25' },
		{ name = 'rockColor1', desc = 'Primary rock color (warm brown)', type = 'hexcolor', default = '#B5835A' },
		{ name = 'rockColor2', desc = 'Rock shadow color (darker brown)', type = 'hexcolor', default = '#8B6243' },
		{ name = 'rockColor3', desc = 'Rock highlight color (sandy tan)', type = 'hexcolor', default = '#D4A574' },
		{ name = 'topColor', desc = 'Flat top color (lighter tan)', type = 'hexcolor', default = '#DEB887' },
		{ name = 'sandColor', desc = 'Sandy base color', type = 'hexcolor', default = '#DEB887' },
		{ name = 'sandColor2', desc = 'Sand accent / darker patches', type = 'hexcolor', default = '#C4A07A' },
		{ name = 'pebbleColor', desc = 'Scattered pebble color', type = 'hexcolor', default = '#9B7653' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a desert rock formation with columnar sandstone pillars, flat tops and a sandy base'
end

local function getCenterBottom(region)
	local mins = region:mins()
	local maxs = region:maxs()
	return g_ivec3.new(
		math.floor((mins.x + maxs.x) / 2),
		mins.y,
		math.floor((mins.z + maxs.z) / 2)
	)
end

-- Build a single rectangular/columnar rock pillar with color variation
local function createPillar(volume, baseX, baseY, baseZ, width, depth, height,
	rockColor1, rockColor2, rockColor3, topColor)

	-- Slight width variation along height for natural look
	for y = 0, height - 1 do
		local t = y / math.max(1, height - 1)
		-- Pillars are slightly wider at the base, narrow a tiny bit at top
		local wMod = 0
		if t < 0.15 then
			-- Base flare
			wMod = math.floor((0.15 - t) * 4)
		elseif t > 0.9 then
			-- Slight top erosion
			wMod = -math.random(0, 1)
		end
		local w = math.max(1, width + wMod)
		local d = math.max(1, depth + wMod)

		-- Color selection: vary by height and add random streaks
		local col
		local rnd = math.random()
		if t > 0.92 then
			-- Top surfaces get the highlight/top color
			col = topColor
		elseif rnd < 0.15 then
			-- Dark shadow streaks (vertical crevices)
			col = rockColor2
		elseif rnd < 0.35 then
			-- Sandy highlights
			col = rockColor3
		else
			col = rockColor1
		end

		-- Fill a rectangular slice at this height
		local halfW = math.floor(w / 2)
		local halfD = math.floor(d / 2)
		for dx = -halfW, halfW do
			for dz = -halfD, halfD do
				local px = baseX + dx
				local py = baseY + y
				local pz = baseZ + dz
				local voxPos = g_ivec3.new(px, py, pz)

				-- Apply dark edge streaks on the faces for depth
				local edgeCol = col
				if (math.abs(dx) == halfW or math.abs(dz) == halfD) and math.random() < 0.3 then
					edgeCol = rockColor2
				end
				g_shape.line(volume, voxPos, voxPos, edgeCol, 1)
			end
		end
	end

	-- Flat top cap
	local halfW = math.floor(width / 2)
	local halfD = math.floor(depth / 2)
	for dx = -halfW, halfW do
		for dz = -halfD, halfD do
			local topPos = g_ivec3.new(baseX + dx, baseY + height, baseZ + dz)
			local col = topColor
			if math.random() < 0.2 then col = rockColor3 end
			g_shape.line(volume, topPos, topPos, col, 1)
		end
	end
end

-- Create the sandy base with slight undulation
local function createSandBase(volume, center, radius, baseHeight, sandColor, sandColor2)
	for dy = 0, baseHeight - 1 do
		local layerR = radius - dy -- Slightly smaller as we go up for a mound look
		local pos = g_ivec3.new(center.x, center.y + dy, center.z)
		local col = sandColor
		if dy == 0 then col = sandColor2 end
		g_shape.dome(volume, pos, 'y', false, layerR * 2, 1, layerR * 2, col)
	end
	-- Add some random color patches on the surface
	for _ = 1, radius do
		local angle = math.random() * 2 * math.pi
		local dist = math.random(1, radius - 1)
		local px = math.floor(center.x + math.cos(angle) * dist)
		local pz = math.floor(center.z + math.sin(angle) * dist)
		local patchPos = g_ivec3.new(px, center.y + baseHeight - 1, pz)
		local col = sandColor
		if math.random() > 0.5 then col = sandColor2 end
		g_shape.line(volume, patchPos, patchPos, col, 1)
	end
end

-- Scatter small rocks/pebbles around the base
local function scatterPebbles(volume, center, baseY, innerRadius, outerRadius,
	numPebbles, pebbleColor, rockColor2)
	for _ = 1, numPebbles do
		local angle = math.random() * 2 * math.pi
		local dist = innerRadius + math.random(0, outerRadius - innerRadius)
		local px = math.floor(center.x + math.cos(angle) * dist)
		local pz = math.floor(center.z + math.sin(angle) * dist)
		local pebbleSize = math.random(1, 3)
		local py = baseY

		local col = pebbleColor
		if math.random() > 0.6 then col = rockColor2 end

		if pebbleSize == 1 then
			-- Single voxel pebble
			g_shape.line(volume, g_ivec3.new(px, py, pz), g_ivec3.new(px, py, pz), col, 1)
		elseif pebbleSize == 2 then
			-- Small 2-3 voxel rock
			g_shape.line(volume, g_ivec3.new(px, py, pz), g_ivec3.new(px + 1, py, pz), col, 1)
			if math.random() > 0.5 then
				g_shape.line(volume, g_ivec3.new(px, py + 1, pz), g_ivec3.new(px, py + 1, pz), col, 1)
			end
		else
			-- Small boulder
			local bPos = g_ivec3.new(px, py, pz)
			g_shape.dome(volume, bPos, 'y', false, 3, 2, 3, col)
		end
	end
end

function main(node, region, color, numPillars, maxHeight, minHeight, pillarWidth,
	clusterRadius, baseRadius, baseHeight, numPebbles, rockColor1, rockColor2,
	rockColor3, topColor, sandColor, sandColor2, pebbleColor, seed)

	if seed == 0 then
		math.randomseed(os.time())
	else
		math.randomseed(seed)
	end

	local volume = node:volume()
	local center = getCenterBottom(region)

	-- Sandy ground base
	createSandBase(volume, center, baseRadius, baseHeight, sandColor, sandColor2)

	local pillarBaseY = center.y + baseHeight - 1

	-- Generate pillar positions — taller pillars tend toward center
	local pillars = {}
	for _ = 1, numPillars do
		-- Distribute in a roughly clustered pattern
		local angle = math.random() * 2 * math.pi
		-- Bias toward center: use square root for uniform area distribution
		local dist = math.sqrt(math.random()) * clusterRadius
		local px = math.floor(center.x + math.cos(angle) * dist)
		local pz = math.floor(center.z + math.sin(angle) * dist)

		-- Height: taller toward center, shorter at edges
		local centerDist = dist / clusterRadius
		local heightRange = maxHeight - minHeight
		local h = math.floor(maxHeight - centerDist * heightRange * 0.7 + math.random(-3, 3))
		h = math.max(minHeight, math.min(maxHeight, h))

		-- Width: slight random variation
		local w = pillarWidth + math.random(-1, 1)
		w = math.max(2, w)
		local d = pillarWidth + math.random(-1, 1)
		d = math.max(2, d)

		pillars[#pillars + 1] = { x = px, z = pz, height = h, width = w, depth = d }
	end

	-- Sort by height so shorter pillars render first (taller ones overlap naturally)
	table.sort(pillars, function(a, b) return a.height < b.height end)

	-- Render all pillars
	for _, p in ipairs(pillars) do
		createPillar(volume, p.x, pillarBaseY, p.z, p.width, p.depth, p.height,
			rockColor1, rockColor2, rockColor3, topColor)
	end

	-- Rubble and detritus at the base of the formation
	-- Small step-like broken rock pieces around the pillars
	local rubbleCount = math.floor(numPillars * 1.5)
	for _ = 1, rubbleCount do
		local angle = math.random() * 2 * math.pi
		local dist = clusterRadius * 0.6 + math.random() * clusterRadius * 0.6
		local rx = math.floor(center.x + math.cos(angle) * dist)
		local rz = math.floor(center.z + math.sin(angle) * dist)
		local rh = math.random(1, math.max(1, math.floor(minHeight * 0.4)))
		local rw = math.random(1, math.max(1, math.floor(pillarWidth * 0.6)))

		local col = rockColor1
		if math.random() > 0.5 then col = rockColor2 end

		-- Small broken column fragment
		for ry = 0, rh do
			for dx = 0, rw do
				for dz = 0, rw do
					local voxPos = g_ivec3.new(rx + dx, pillarBaseY + ry, rz + dz)
					local vCol = col
					if ry == rh then vCol = topColor end
					if math.random() > 0.15 then
						g_shape.line(volume, voxPos, voxPos, vCol, 1)
					end
				end
			end
		end
	end

	-- Scattered pebbles around the outer base
	scatterPebbles(volume, center, pillarBaseY, clusterRadius, baseRadius,
		numPebbles, pebbleColor, rockColor2)
end
