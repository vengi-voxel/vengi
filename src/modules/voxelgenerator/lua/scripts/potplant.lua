--
-- Pot plant generator
-- Creates a variety of randomized plants in a decorative pot.
-- Supports multiple plant types: succulent, fern, flower, cactus, bush.
-- Each type has its own characteristic shape and growth pattern.
--

function arguments()
	return {
		{ name = 'plantType', desc = 'Type of plant to generate', type = 'enum', default = 'succulent',
		  enum = 'succulent,fern,flower,cactus,bush' },
		{ name = 'plantHeight', desc = 'Height of the plant above the pot', type = 'int', default = '12', min = '4', max = '40' },
		{ name = 'plantWidth', desc = 'Width/spread of the plant', type = 'int', default = '10', min = '3', max = '30' },
		{ name = 'density', desc = 'Density of foliage (1=sparse, 5=dense)', type = 'int', default = '3', min = '1', max = '5' },
		{ name = 'potStyle', desc = 'Style of the pot', type = 'enum', default = 'round',
		  enum = 'round,square,tall,wide,bowl' },
		{ name = 'potHeight', desc = 'Height of the pot', type = 'int', default = '4', min = '2', max = '10' },
		{ name = 'potRadius', desc = 'Radius of the pot', type = 'int', default = '6', min = '3', max = '15' },
		{ name = 'flowers', desc = 'Number of flowers (for flower type) or buds', type = 'int', default = '5', min = '0', max = '20' },
		{ name = 'leafColor', desc = 'Color of the leaves/plant body', type = 'colorindex', default = '2' },
		{ name = 'flowerColor', desc = 'Color of flowers or accents', type = 'colorindex', default = '4' },
		{ name = 'potColor', desc = 'Color of the pot', type = 'colorindex', default = '3' },
		{ name = 'soilColor', desc = 'Color of the soil', type = 'colorindex', default = '1' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Generates a randomized plant in a decorative pot. Supports succulent, fern, flower, cactus, and bush types.'
end

-- Utility: lerp
local function lerp(a, b, t)
	return a + (b - a) * t
end

-- Draw a single voxel if it is within the volume region
local function safeSetVoxel(volume, region, x, y, z, c)
	local mins = region:mins()
	local maxs = region:maxs()
	if x >= mins.x and x <= maxs.x and y >= mins.y and y <= maxs.y and z >= mins.z and z <= maxs.z then
		volume:setVoxel(x, y, z, c)
	end
end

-- Draw a filled sphere of voxels
local function drawSphere(volume, region, cx, cy, cz, radius, c)
	local r2 = radius * radius
	local ri = math.ceil(radius)
	for dy = -ri, ri do
		for dx = -ri, ri do
			for dz = -ri, ri do
				if dx * dx + dy * dy + dz * dz <= r2 then
					safeSetVoxel(volume, region, cx + dx, cy + dy, cz + dz, c)
				end
			end
		end
	end
end

-- Draw a pot based on the selected style
local function drawPot(volume, region, cx, baseY, cz, style, potRadius, potHeight, potColor)
	if style == 'round' then
		-- Standard round pot with rim
		local bodyPos = g_ivec3.new(cx, baseY, cz)
		g_shape.cylinder(volume, bodyPos, 'y', potRadius, potHeight, potColor)
		-- Rim
		local rimPos = g_ivec3.new(cx, baseY + potHeight - 1, cz)
		g_shape.cylinder(volume, rimPos, 'y', potRadius + 1, 1, potColor)
		-- Hollow inside
		local innerRadius = potRadius - 1
		if innerRadius > 1 then
			local hollowPos = g_ivec3.new(cx, baseY + 1, cz)
			g_shape.cylinder(volume, hollowPos, 'y', innerRadius, potHeight, -1)
		end
		-- Small foot
		g_shape.cylinder(volume, g_ivec3.new(cx, baseY, cz), 'y', math.max(2, potRadius - 2), 1, potColor)

	elseif style == 'square' then
		-- Square pot using cube
		local halfW = potRadius
		local pos = g_ivec3.new(cx - halfW, baseY, cz - halfW)
		g_shape.cube(volume, pos, halfW * 2 + 1, potHeight, halfW * 2 + 1, potColor)
		-- Rim
		local rimPos = g_ivec3.new(cx - halfW - 1, baseY + potHeight - 1, cz - halfW - 1)
		g_shape.cube(volume, rimPos, halfW * 2 + 3, 1, halfW * 2 + 3, potColor)
		-- Hollow inside
		if halfW > 1 then
			local innerHalf = halfW - 1
			local hollowPos = g_ivec3.new(cx - innerHalf, baseY + 1, cz - innerHalf)
			g_shape.cube(volume, hollowPos, innerHalf * 2 + 1, potHeight, innerHalf * 2 + 1, -1)
		end

	elseif style == 'tall' then
		-- Tall narrow pot
		local narrowRadius = math.max(2, math.floor(potRadius * 0.7))
		local tallHeight = potHeight + math.max(2, math.floor(potHeight * 0.5))
		local bodyPos = g_ivec3.new(cx, baseY, cz)
		g_shape.cylinder(volume, bodyPos, 'y', narrowRadius, tallHeight, potColor)
		-- Wider rim
		local rimPos = g_ivec3.new(cx, baseY + tallHeight - 1, cz)
		g_shape.cylinder(volume, rimPos, 'y', narrowRadius + 1, 1, potColor)
		-- Hollow
		local innerRadius = narrowRadius - 1
		if innerRadius > 1 then
			local hollowPos = g_ivec3.new(cx, baseY + 1, cz)
			g_shape.cylinder(volume, hollowPos, 'y', innerRadius, tallHeight, -1)
		end
		-- Foot
		g_shape.cylinder(volume, g_ivec3.new(cx, baseY, cz), 'y', math.max(2, narrowRadius - 1), 1, potColor)

	elseif style == 'wide' then
		-- Wide shallow pot
		local wideRadius = potRadius + math.max(2, math.floor(potRadius * 0.4))
		local shallowHeight = math.max(2, math.floor(potHeight * 0.6))
		local bodyPos = g_ivec3.new(cx, baseY, cz)
		g_shape.cylinder(volume, bodyPos, 'y', wideRadius, shallowHeight, potColor)
		-- Rim
		local rimPos = g_ivec3.new(cx, baseY + shallowHeight - 1, cz)
		g_shape.cylinder(volume, rimPos, 'y', wideRadius + 1, 1, potColor)
		-- Hollow
		local innerRadius = wideRadius - 1
		if innerRadius > 1 then
			local hollowPos = g_ivec3.new(cx, baseY + 1, cz)
			g_shape.cylinder(volume, hollowPos, 'y', innerRadius, shallowHeight, -1)
		end

	elseif style == 'bowl' then
		-- Bowl-shaped pot using dome
		local bodyPos = g_ivec3.new(cx, baseY, cz)
		local bowlDiameter = potRadius * 2
		g_shape.dome(volume, bodyPos, 'y', false, bowlDiameter, potHeight, bowlDiameter, potColor)
		-- Hollow interior using a smaller inverted fill
		local innerDiameter = bowlDiameter - 2
		if innerDiameter > 2 then
			local hollowPos = g_ivec3.new(cx, baseY + 1, cz)
			g_shape.dome(volume, hollowPos, 'y', false, innerDiameter, potHeight, innerDiameter, -1)
		end
		-- Flat bottom
		g_shape.cylinder(volume, g_ivec3.new(cx, baseY, cz), 'y', math.max(2, potRadius - 2), 1, potColor)
	end
end

-- Returns the actual height of the pot (some styles modify it)
local function getPotActualHeight(style, potHeight)
	if style == 'tall' then
		return potHeight + math.max(2, math.floor(potHeight * 0.5))
	elseif style == 'wide' then
		return math.max(2, math.floor(potHeight * 0.6))
	end
	return potHeight
end

-- Draw soil on top of the pot
local function drawSoil(volume, cx, soilY, cz, style, potRadius, soilColor)
	local soilRadius
	if style == 'wide' then
		soilRadius = potRadius + math.max(2, math.floor(potRadius * 0.4)) - 1
	elseif style == 'tall' then
		soilRadius = math.max(2, math.floor(potRadius * 0.7)) - 1
	elseif style == 'square' then
		-- Fill square soil
		local innerHalf = potRadius - 1
		if innerHalf > 0 then
			local soilPos = g_ivec3.new(cx - innerHalf, soilY, cz - innerHalf)
			g_shape.cube(volume, soilPos, innerHalf * 2 + 1, 1, innerHalf * 2 + 1, soilColor)
		end
		return
	elseif style == 'bowl' then
		soilRadius = potRadius - 1
	else
		soilRadius = potRadius - 1
	end
	if soilRadius > 0 then
		g_shape.cylinder(volume, g_ivec3.new(cx, soilY, cz), 'y', soilRadius, 1, soilColor)
	end
end

-- Succulent: rosette of thick rounded leaves radiating from center
local function generateSucculent(volume, region, cx, baseY, cz, plantHeight, plantWidth, density, leafColor, flowerColor, numFlowers)
	local layers = math.max(2, math.floor(plantHeight / 3))
	local leavesPerLayer = 4 + density
	local palette = { leafColor }
	-- Add slightly varied colors
	local similarColors = node_palette:similar(leafColor, 3)
	for _, c in ipairs(similarColors) do
		palette[#palette + 1] = c
	end

	for layer = 1, layers do
		local t = (layer - 1) / math.max(1, layers - 1)
		local layerY = baseY + math.floor(t * plantHeight * 0.6)
		local layerRadius = math.floor(lerp(plantWidth * 0.5, plantWidth * 0.2, t))
		local angleOffset = layer * 0.4

		for i = 1, leavesPerLayer do
			local angle = angleOffset + (i - 1) * (2 * math.pi / leavesPerLayer) + (math.random() - 0.5) * 0.3
			local dx = math.cos(angle)
			local dz = math.sin(angle)
			-- Each succulent leaf is a short thick line with a sphere at end
			local leafLen = layerRadius + math.random(-1, 1)
			local endX = math.floor(cx + dx * leafLen)
			local endZ = math.floor(cz + dz * leafLen)
			local endY = layerY + math.random(0, 1)
			local c = palette[math.random(1, #palette)]
			local startPos = g_ivec3.new(cx, layerY, cz)
			local endPos = g_ivec3.new(endX, endY, endZ)
			g_shape.line(volume, startPos, endPos, c, math.max(1, math.floor(density * 0.5)))
			-- Rounded tip
			drawSphere(volume, region, endX, endY, endZ, math.max(1, math.floor(density * 0.4) + 1), c)
		end
		coroutine.yield()
	end

	-- Central rosette top
	drawSphere(volume, region, cx, baseY + math.floor(plantHeight * 0.6) + 1, cz, math.max(1, math.floor(density * 0.6) + 1), leafColor)

	-- Flowers (small dots on top)
	for _ = 1, numFlowers do
		local angle = math.random() * 2 * math.pi
		local dist = math.random(1, math.max(1, math.floor(plantWidth * 0.3)))
		local fx = math.floor(cx + math.cos(angle) * dist)
		local fz = math.floor(cz + math.sin(angle) * dist)
		local fy = baseY + math.floor(plantHeight * 0.6) + math.random(1, 3)
		safeSetVoxel(volume, region, fx, fy, fz, flowerColor)
		safeSetVoxel(volume, region, fx, fy + 1, fz, flowerColor)
	end
end

-- Fern: arching fronds radiating from center
local function generateFern(volume, region, cx, baseY, cz, plantHeight, plantWidth, density, leafColor, flowerColor, numFlowers)
	local numFronds = 3 + density
	local palette = { leafColor }
	local similarColors = node_palette:similar(leafColor, 2)
	for _, c in ipairs(similarColors) do
		palette[#palette + 1] = c
	end

	for i = 1, numFronds do
		local angle = (i - 1) * (2 * math.pi / numFronds) + (math.random() - 0.5) * 0.4
		local dx = math.cos(angle)
		local dz = math.sin(angle)

		-- Each frond curves upward then arches outward and down
		local frondLength = plantHeight + math.random(-2, 2)
		local steps = math.max(6, frondLength * 2)
		local prevX, prevY, prevZ = cx, baseY, cz

		for s = 1, steps do
			local t = s / steps
			-- Parabolic arch: rises quickly, then descends gently
			local arch = 4 * t * (1 - t)
			local spread = t * plantWidth * 0.6
			local curX = math.floor(cx + dx * spread)
			local curZ = math.floor(cz + dz * spread)
			local curY = math.floor(baseY + arch * frondLength * 0.8 + t * 0.5)

			local c = palette[math.random(1, #palette)]
			local startPos = g_ivec3.new(prevX, prevY, prevZ)
			local endPos = g_ivec3.new(curX, curY, curZ)
			g_shape.line(volume, startPos, endPos, c, 1)

			-- Side leaflets along the frond
			if s > 2 and s % math.max(1, 4 - math.floor(density / 2)) == 0 then
				local perpX = -dz
				local perpZ = dx
				for side = -1, 1, 2 do
					local leafLen = math.max(1, math.floor((1 - t) * plantWidth * 0.2))
					local lx = math.floor(curX + perpX * side * leafLen)
					local lz = math.floor(curZ + perpZ * side * leafLen)
					local ly = curY - math.random(0, 1)
					local leafStart = g_ivec3.new(curX, curY, curZ)
					local leafEnd = g_ivec3.new(lx, ly, lz)
					g_shape.line(volume, leafStart, leafEnd, c, 1)
				end
			end

			prevX, prevY, prevZ = curX, curY, curZ
		end
		coroutine.yield()
	end
end

-- Flower: stems with blooms on top
local function generateFlower(volume, region, cx, baseY, cz, plantHeight, plantWidth, density, leafColor, flowerColor, numFlowers)
	numFlowers = math.max(1, numFlowers)
	local palette = { leafColor }
	local similarColors = node_palette:similar(leafColor, 2)
	for _, c in ipairs(similarColors) do
		palette[#palette + 1] = c
	end

	-- Small leaves at base
	local numBaseLeaves = 2 + density
	for i = 1, numBaseLeaves do
		local angle = (i - 1) * (2 * math.pi / numBaseLeaves) + (math.random() - 0.5) * 0.3
		local dx = math.cos(angle)
		local dz = math.sin(angle)
		local leafLen = math.max(2, math.floor(plantWidth * 0.3)) + math.random(-1, 1)
		local startPos = g_ivec3.new(cx, baseY, cz)
		local endPos = g_ivec3.new(
			math.floor(cx + dx * leafLen),
			baseY + math.random(0, 2),
			math.floor(cz + dz * leafLen)
		)
		local ctrl = g_ivec3.new(
			math.floor(cx + dx * leafLen * 0.5),
			baseY + math.random(1, 3),
			math.floor(cz + dz * leafLen * 0.5)
		)
		g_shape.bezier(volume, startPos, endPos, ctrl, palette[math.random(1, #palette)], math.max(1, math.floor(density * 0.5)))
	end

	-- Flower stems with blooms
	for i = 1, numFlowers do
		local angle = (i - 1) * (2 * math.pi / numFlowers) + (math.random() - 0.5) * 0.5
		local dx = math.cos(angle)
		local dz = math.sin(angle)

		local stemHeight = plantHeight + math.random(-2, 2)
		local spread = math.random(0, math.max(0, math.floor(plantWidth * 0.2)))
		local topX = math.floor(cx + dx * spread)
		local topZ = math.floor(cz + dz * spread)
		local topY = baseY + stemHeight

		-- Stem using bezier for slight curve
		local startPos = g_ivec3.new(cx, baseY, cz)
		local endPos = g_ivec3.new(topX, topY, topZ)
		local ctrlX = math.floor(cx + dx * spread * 0.5 + (math.random() - 0.5) * 2)
		local ctrlZ = math.floor(cz + dz * spread * 0.5 + (math.random() - 0.5) * 2)
		local ctrl = g_ivec3.new(ctrlX, baseY + math.floor(stemHeight * 0.6), ctrlZ)
		g_shape.bezier(volume, startPos, endPos, ctrl, leafColor, 1)

		-- Flower head: small dome or sphere
		local petalRadius = math.max(1, math.floor(density * 0.5) + 1)
		drawSphere(volume, region, topX, topY, topZ, petalRadius, flowerColor)

		-- Center dot (different color approx: use leaf color for center)
		safeSetVoxel(volume, region, topX, topY + 1, topZ, leafColor)

		-- Small petals around the bloom
		if petalRadius >= 2 then
			for p = 0, 3 do
				local pAngle = p * math.pi * 0.5
				local px = math.floor(topX + math.cos(pAngle) * (petalRadius + 1))
				local pz = math.floor(topZ + math.sin(pAngle) * (petalRadius + 1))
				safeSetVoxel(volume, region, px, topY, pz, flowerColor)
			end
		end
		coroutine.yield()
	end
end

-- Cactus: columnar shapes, sometimes branching
local function generateCactus(volume, region, cx, baseY, cz, plantHeight, plantWidth, density, leafColor, flowerColor, numFlowers)
	-- Main central column
	local mainRadius = math.max(1, math.floor(plantWidth * 0.15))
	local mainPos = g_ivec3.new(cx, baseY, cz)
	g_shape.cylinder(volume, mainPos, 'y', mainRadius, plantHeight, leafColor)
	-- Rounded top
	local topPos = g_ivec3.new(cx, baseY + plantHeight - 1, cz)
	g_shape.dome(volume, topPos, 'y', false, mainRadius * 2, math.max(1, mainRadius), mainRadius * 2, leafColor)

	-- Side arms (branches)
	local numArms = math.max(0, density - 1)
	for i = 1, numArms do
		local angle = (i - 1) * (2 * math.pi / math.max(1, numArms)) + (math.random() - 0.5) * 0.5
		local dx = math.cos(angle)
		local dz = math.sin(angle)
		local armY = baseY + math.random(math.floor(plantHeight * 0.3), math.floor(plantHeight * 0.7))
		local armLength = math.max(2, math.floor(plantWidth * 0.25)) + math.random(-1, 1)
		local armRadius = math.max(1, mainRadius - 1)

		-- Horizontal segment
		local armStartPos = g_ivec3.new(cx, armY, cz)
		local armMidX = math.floor(cx + dx * armLength)
		local armMidZ = math.floor(cz + dz * armLength)
		local armMidPos = g_ivec3.new(armMidX, armY, armMidZ)
		g_shape.line(volume, armStartPos, armMidPos, leafColor, armRadius)

		-- Vertical segment going up
		local armTopHeight = math.random(math.max(2, math.floor(plantHeight * 0.2)), math.max(3, math.floor(plantHeight * 0.5)))
		local armVertPos = g_ivec3.new(armMidX, armY, armMidZ)
		g_shape.cylinder(volume, armVertPos, 'y', armRadius, armTopHeight, leafColor)

		-- Rounded top of arm
		local armTopPos = g_ivec3.new(armMidX, armY + armTopHeight - 1, armMidZ)
		g_shape.dome(volume, armTopPos, 'y', false, armRadius * 2, math.max(1, armRadius), armRadius * 2, leafColor)
		coroutine.yield()
	end

	-- Ridges along the main column (vertical lines on surface)
	if density >= 3 then
		local ridgeColor = leafColor
		local ridgeAngles = { 0, math.pi * 0.5, math.pi, math.pi * 1.5 }
		for _, a in ipairs(ridgeAngles) do
			local rx = math.floor(cx + math.cos(a) * (mainRadius + 1))
			local rz = math.floor(cz + math.sin(a) * (mainRadius + 1))
			for ry = baseY, baseY + plantHeight - 1 do
				safeSetVoxel(volume, region, rx, ry, rz, ridgeColor)
			end
		end
	end

	-- Small flowers/buds on top
	for _ = 1, numFlowers do
		local angle = math.random() * 2 * math.pi
		local fx = math.floor(cx + math.cos(angle) * math.random(0, mainRadius))
		local fz = math.floor(cz + math.sin(angle) * math.random(0, mainRadius))
		local fy = baseY + plantHeight + math.random(0, 1)
		safeSetVoxel(volume, region, fx, fy, fz, flowerColor)
		-- Small extra petal voxels
		safeSetVoxel(volume, region, fx + 1, fy, fz, flowerColor)
		safeSetVoxel(volume, region, fx - 1, fy, fz, flowerColor)
		safeSetVoxel(volume, region, fx, fy, fz + 1, flowerColor)
		safeSetVoxel(volume, region, fx, fy, fz - 1, flowerColor)
	end
end

-- Bush: dense rounded mass of foliage
local function generateBush(volume, region, cx, baseY, cz, plantHeight, plantWidth, density, leafColor, flowerColor, numFlowers)
	local palette = { leafColor }
	local similarColors = node_palette:similar(leafColor, 4)
	for _, c in ipairs(similarColors) do
		palette[#palette + 1] = c
	end

	-- Small woody stem(s)
	local numStems = math.max(1, math.floor(density * 0.5) + 1)
	local stemHeight = math.max(2, math.floor(plantHeight * 0.3))
	for i = 1, numStems do
		local angle = (i - 1) * (2 * math.pi / numStems) + (math.random() - 0.5) * 0.3
		local dx = math.cos(angle)
		local dz = math.sin(angle)
		local stemSpread = math.random(0, math.max(0, math.floor(plantWidth * 0.1)))
		local startPos = g_ivec3.new(cx, baseY, cz)
		local topX = math.floor(cx + dx * stemSpread)
		local topZ = math.floor(cz + dz * stemSpread)
		local endPos = g_ivec3.new(topX, baseY + stemHeight, topZ)
		-- Use soil/trunk-ish color (reuse first palette color or leafColor)
		g_shape.line(volume, startPos, endPos, leafColor, 1)
	end

	-- Multiple overlapping domes/ellipses for bushy foliage
	local numClusters = density + 2
	for _ = 1, numClusters do
		local angle = math.random() * 2 * math.pi
		local dist = math.random(0, math.max(1, math.floor(plantWidth * 0.2)))
		local clX = math.floor(cx + math.cos(angle) * dist)
		local clZ = math.floor(cz + math.sin(angle) * dist)
		local clY = baseY + stemHeight + math.random(-1, math.max(0, math.floor(plantHeight * 0.1)))
		local clRadius = math.max(2, math.floor(plantWidth * 0.35) + math.random(-1, 1))
		local clHeight = math.max(2, math.floor(plantHeight * 0.5) + math.random(-1, 1))
		local c = palette[math.random(1, #palette)]
		local clPos = g_ivec3.new(clX, clY, clZ)
		g_shape.dome(volume, clPos, 'y', false, clRadius * 2, clHeight, clRadius * 2, c)
		-- Small inverted dome for bottom volume
		if clRadius > 2 then
			local underPos = g_ivec3.new(clX, clY, clZ)
			g_shape.dome(volume, underPos, 'y', true, math.floor(clRadius * 1.4), math.max(1, math.floor(clHeight * 0.3)), math.floor(clRadius * 1.4), c)
		end
		coroutine.yield()
	end

	-- Scatter small flowers/berries
	for _ = 1, numFlowers do
		local angle = math.random() * 2 * math.pi
		local dist = math.random(0, math.max(1, math.floor(plantWidth * 0.35)))
		local fx = math.floor(cx + math.cos(angle) * dist)
		local fz = math.floor(cz + math.sin(angle) * dist)
		local fy = baseY + stemHeight + math.random(0, math.max(1, math.floor(plantHeight * 0.4)))
		safeSetVoxel(volume, region, fx, fy, fz, flowerColor)
	end
end

function main(node, region, color, plantType, plantHeight, plantWidth, density, potStyle,
	potHeight, potRadius, flowers, leafColor, flowerColor, potColor, soilColor, seed)

	if seed == 0 then
		math.randomseed(os.time())
	else
		math.randomseed(seed)
	end

	-- Store palette globally for helper functions
	node_palette = node:palette()

	local volume = node:volume()
	local pos = region:mins()
	local cx = pos.x + math.floor(region:width() / 2)
	local cz = pos.z + math.floor(region:depth() / 2)
	local baseY = pos.y

	-- Draw the pot
	drawPot(volume, region, cx, baseY, cz, potStyle, potRadius, potHeight, potColor)

	-- Calculate soil/plant base position
	local actualPotHeight = getPotActualHeight(potStyle, potHeight)
	local soilY = baseY + actualPotHeight - 1
	drawSoil(volume, cx, soilY, cz, potStyle, potRadius, soilColor)

	local plantBaseY = soilY + 1

	-- Generate the plant
	if plantType == 'succulent' then
		generateSucculent(volume, region, cx, plantBaseY, cz, plantHeight, plantWidth, density, leafColor, flowerColor, flowers)
	elseif plantType == 'fern' then
		generateFern(volume, region, cx, plantBaseY, cz, plantHeight, plantWidth, density, leafColor, flowerColor, flowers)
	elseif plantType == 'flower' then
		generateFlower(volume, region, cx, plantBaseY, cz, plantHeight, plantWidth, density, leafColor, flowerColor, flowers)
	elseif plantType == 'cactus' then
		generateCactus(volume, region, cx, plantBaseY, cz, plantHeight, plantWidth, density, leafColor, flowerColor, flowers)
	elseif plantType == 'bush' then
		generateBush(volume, region, cx, plantBaseY, cz, plantHeight, plantWidth, density, leafColor, flowerColor, flowers)
	end
end
