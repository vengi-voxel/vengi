--
-- Palm tree generator with multiple varieties
-- Supports coconut, royal, fan, and date palm styles with curved trunks,
-- natural fronds, optional fruit clusters, and bark ring texture.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'variety', desc = 'Palm variety: coconut, royal, fan, date', type = 'enum', default = 'coconut',
		  enum = 'coconut,royal,fan,date' },
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '20', min = '6', max = '50' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk base', type = 'int', default = '2', min = '1', max = '5' },
		{ name = 'trunkCurve', desc = 'Horizontal curve amount', type = 'int', default = '4', min = '0', max = '15' },
		{ name = 'trunkLean', desc = 'Lean direction angle (degrees, 0=random)', type = 'int', default = '0', min = '0', max = '360' },
		{ name = 'fronds', desc = 'Number of fronds', type = 'int', default = '8', min = '4', max = '16' },
		{ name = 'frondLength', desc = 'Length of fronds', type = 'int', default = '10', min = '4', max = '20' },
		{ name = 'frondDroop', desc = 'How much fronds droop at tips', type = 'int', default = '6', min = '1', max = '14' },
		{ name = 'frondWidth', desc = 'Width (thickness) of frond leaves', type = 'int', default = '1', min = '1', max = '3' },
		{ name = 'fruits', desc = 'Show fruit cluster at crown', type = 'bool', default = 'true' },
		{ name = 'barkRings', desc = 'Show bark ring texture on trunk', type = 'bool', default = 'true' },
		{ name = 'trunkColor', desc = 'Color of the trunk', type = 'hexcolor', default = '#8B7355' },
		{ name = 'barkColor', desc = 'Color of bark rings', type = 'hexcolor', default = '#6B4226' },
		{ name = 'leavesColor', desc = 'Primary frond color', type = 'hexcolor', default = '#2E6B14' },
		{ name = 'leavesColor2', desc = 'Secondary frond color (tips)', type = 'hexcolor', default = '#5C8A2F' },
		{ name = 'fruitColor', desc = 'Color of fruits', type = 'hexcolor', default = '#8B6914' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a palm tree with selectable variety (coconut, royal, fan, date), curved trunk, natural fronds and optional fruits'
end

-- Draw a bezier curve with tapering thickness, returning final position
local function drawBezier(volume, startPos, endPos, control, startThickness, endThickness, steps, voxelColor)
	local last = startPos
	for i = 1, steps do
		local t = i / steps
		local invT = 1.0 - t
		local p = g_ivec3.new(
			math.floor(invT * invT * startPos.x + 2 * invT * t * control.x + t * t * endPos.x),
			math.floor(invT * invT * startPos.y + 2 * invT * t * control.y + t * t * endPos.y),
			math.floor(invT * invT * startPos.z + 2 * invT * t * control.z + t * t * endPos.z)
		)
		local thickness = math.max(1, math.ceil(startThickness + (endThickness - startThickness) * t))
		g_shape.line(volume, last, p, voxelColor, thickness)
		last = p
	end
	return last
end

-- Create a curved palm trunk with optional bark rings, returning top position and lean direction
local function createTrunk(volume, basePos, trunkHeight, trunkStrength, trunkCurve, leanAngle, trunkColor, barkColor, barkRings, variety)
	-- Lean direction
	local dir
	if leanAngle == 0 then
		dir = math.random() * 2 * math.pi
	else
		dir = math.rad(leanAngle)
	end

	local curveX = math.floor(math.cos(dir) * trunkCurve)
	local curveZ = math.floor(math.sin(dir) * trunkCurve)

	local topPos = g_ivec3.new(
		basePos.x + curveX,
		basePos.y + trunkHeight,
		basePos.z + curveZ
	)

	-- Control point for the trunk curve — royal palms are straighter, coconut leans more
	local ctrlHeight
	if variety == 'royal' then
		ctrlHeight = 0.6
	elseif variety == 'coconut' then
		ctrlHeight = 0.35
	else
		ctrlHeight = 0.45
	end
	local ctrl = g_ivec3.new(
		basePos.x + math.floor(curveX * 0.3),
		basePos.y + math.floor(trunkHeight * ctrlHeight),
		basePos.z + math.floor(curveZ * 0.3)
	)

	-- Taper — royal palms have a distinctive bulge, others taper normally
	local topThickness = math.max(1, trunkStrength - 1)
	if variety == 'royal' then
		-- Draw lower thicker section then thinner upper
		local midY = math.floor(trunkHeight * 0.4)
		local midPos = g_ivec3.new(
			math.floor(basePos.x + curveX * 0.3),
			basePos.y + midY,
			math.floor(basePos.z + curveZ * 0.3)
		)
		local midCtrl = g_ivec3.new(
			basePos.x + math.floor(curveX * 0.15),
			basePos.y + math.floor(midY * 0.5),
			basePos.z + math.floor(curveZ * 0.15)
		)
		-- Bulging lower trunk
		drawBezier(volume, basePos, midPos, midCtrl, trunkStrength + 1, trunkStrength, midY, trunkColor)
		-- Thinner smooth upper trunk (crownshaft)
		local upperCtrl = g_ivec3.new(
			math.floor((midPos.x + topPos.x) / 2),
			math.floor((midPos.y + topPos.y) / 2),
			math.floor((midPos.z + topPos.z) / 2)
		)
		drawBezier(volume, midPos, topPos, upperCtrl, trunkStrength, topThickness, trunkHeight - midY, trunkColor)
	elseif variety == 'date' then
		-- Date palms are thick and fairly straight
		drawBezier(volume, basePos, topPos, ctrl, trunkStrength + 1, trunkStrength, trunkHeight, trunkColor)
	else
		drawBezier(volume, basePos, topPos, ctrl, trunkStrength, topThickness, trunkHeight, trunkColor)
	end

	-- Bark rings — horizontal ring texture along the trunk
	if barkRings then
		local ringSpacing
		if variety == 'date' then
			ringSpacing = 2
		else
			ringSpacing = 3
		end
		for h = ringSpacing, trunkHeight - 2, ringSpacing do
			local t = h / trunkHeight
			local invT = 1.0 - t
			local ringPos = g_ivec3.new(
				math.floor(invT * invT * basePos.x + 2 * invT * t * ctrl.x + t * t * topPos.x),
				math.floor(invT * invT * basePos.y + 2 * invT * t * ctrl.y + t * t * topPos.y),
				math.floor(invT * invT * basePos.z + 2 * invT * t * ctrl.z + t * t * topPos.z)
			)
			-- Thin ring around the trunk
			local ringThick = math.max(1, trunkStrength)
			g_shape.cylinder(volume, ringPos, 'y', ringThick, 1, barkColor)
		end
	end

	-- Slight base bulge
	g_shape.dome(volume, basePos, 'y', false,
		(trunkStrength + 1) * 2, math.max(1, math.floor(trunkStrength * 0.4)), (trunkStrength + 1) * 2, trunkColor)

	return topPos, dir
end

-- Create a single frond using bezier curves with leaf segments
local function createFrond(volume, origin, angle, frondLength, frondDroop, frondWidth, leavesColor, leavesColor2, variety)
	local dx = math.cos(angle)
	local dz = math.sin(angle)

	-- Frond rises slightly then arches outward and droops
	local riseHeight
	if variety == 'fan' then
		riseHeight = math.random(2, math.max(3, math.floor(frondLength * 0.5)))
	else
		riseHeight = math.random(1, math.max(2, math.floor(frondLength * 0.3)))
	end

	local tipX = math.floor(origin.x + dx * frondLength)
	local tipZ = math.floor(origin.z + dz * frondLength)
	local tipY = origin.y - frondDroop

	local endPos = g_ivec3.new(tipX, tipY, tipZ)
	local ctrlPos = g_ivec3.new(
		math.floor(origin.x + dx * frondLength * 0.4),
		origin.y + riseHeight,
		math.floor(origin.z + dz * frondLength * 0.4)
	)

	local steps = math.max(6, frondLength)

	if variety == 'fan' then
		-- Fan palms: spread into a fan shape with multiple sub-fronds
		local fanSpread = math.max(3, math.floor(frondLength * 0.6))
		local numFingers = math.random(5, 8)
		local fanAngleRange = math.pi * 0.5 -- 90 degree fan spread
		for f = 1, numFingers do
			local fingerAngle = angle - fanAngleRange / 2 + (f - 1) * fanAngleRange / (numFingers - 1)
			local fDx = math.cos(fingerAngle)
			local fDz = math.sin(fingerAngle)
			local fingerLen = fanSpread + math.random(-1, 2)
			local fingerEnd = g_ivec3.new(
				math.floor(origin.x + fDx * fingerLen),
				origin.y + riseHeight - math.random(1, math.max(2, math.floor(frondDroop * 0.5))),
				math.floor(origin.z + fDz * fingerLen)
			)
			local fingerCtrl = g_ivec3.new(
				math.floor(origin.x + fDx * fingerLen * 0.4),
				origin.y + riseHeight + 1,
				math.floor(origin.z + fDz * fingerLen * 0.4)
			)
			local col = leavesColor
			if math.random() > 0.6 then
				col = leavesColor2
			end
			drawBezier(volume, origin, fingerEnd, fingerCtrl, frondWidth, 1, math.max(4, fingerLen), col)
		end
	else
		-- Pinnate fronds (coconut, royal, date): central rachis with leaflets
		-- Draw the main rachis (central stem)
		local last = origin
		for i = 1, steps do
			local t = i / steps
			local invT = 1.0 - t
			local p = g_ivec3.new(
				math.floor(invT * invT * origin.x + 2 * invT * t * ctrlPos.x + t * t * endPos.x),
				math.floor(invT * invT * origin.y + 2 * invT * t * ctrlPos.y + t * t * endPos.y),
				math.floor(invT * invT * origin.z + 2 * invT * t * ctrlPos.z + t * t * endPos.z)
			)
			local col = leavesColor
			if t > 0.75 then
				col = leavesColor2
			end
			g_shape.line(volume, last, p, col, 1)

			-- Add leaflets (small perpendicular lines) along the rachis
			if i > 1 and i % 2 == 0 then
				local perpX = -dz -- perpendicular direction
				local perpZ = dx
				local leafletLen = math.max(1, math.floor((1.0 - t * 0.6) * frondWidth * 2 + 1))
				local leafCol = leavesColor
				if math.random() > 0.7 then
					leafCol = leavesColor2
				end
				-- Leaflet on both sides
				local leafL = g_ivec3.new(
					p.x + math.floor(perpX * leafletLen),
					p.y,
					p.z + math.floor(perpZ * leafletLen)
				)
				local leafR = g_ivec3.new(
					p.x - math.floor(perpX * leafletLen),
					p.y,
					p.z - math.floor(perpZ * leafletLen)
				)
				g_shape.line(volume, p, leafL, leafCol, 1)
				g_shape.line(volume, p, leafR, leafCol, 1)
			end

			last = p
		end
	end
end

-- Create a fruit cluster beneath the crown
local function createFruits(volume, crownPos, numFruits, fruitColor, variety)
	if variety == 'coconut' then
		-- Coconut cluster — a few large fruits hanging just below the crown
		local count = math.random(2, math.min(5, numFruits))
		local angleStep = (2 * math.pi) / count
		local startAngle = math.random() * 2 * math.pi
		for i = 1, count do
			local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.3
			local dist = math.random(1, 2)
			local fx = math.floor(crownPos.x + math.cos(angle) * dist)
			local fz = math.floor(crownPos.z + math.sin(angle) * dist)
			local fy = crownPos.y - math.random(1, 2)
			local fruitPos = g_ivec3.new(fx, fy, fz)
			g_shape.dome(volume, fruitPos, 'y', false, 2, 2, 2, fruitColor)
		end
	elseif variety == 'date' then
		-- Date clusters — hanging bunches
		local clusters = math.random(3, 6)
		local angleStep = (2 * math.pi) / clusters
		local startAngle = math.random() * 2 * math.pi
		for i = 1, clusters do
			local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.3
			local dist = math.random(1, 3)
			local bx = math.floor(crownPos.x + math.cos(angle) * dist)
			local bz = math.floor(crownPos.z + math.sin(angle) * dist)
			local hangLen = math.random(2, 4)
			-- Hanging stem
			local bunchTop = g_ivec3.new(bx, crownPos.y - 1, bz)
			local bunchBot = g_ivec3.new(bx, crownPos.y - 1 - hangLen, bz)
			g_shape.line(volume, bunchTop, bunchBot, fruitColor, 1)
			-- Small dome at the bottom of the bunch
			g_shape.dome(volume, bunchBot, 'y', false, 2, math.max(1, math.floor(hangLen * 0.5)), 2, fruitColor)
		end
	else
		-- Royal/fan — small decorative fruit cluster
		local clusterPos = g_ivec3.new(crownPos.x, crownPos.y - 1, crownPos.z)
		g_shape.dome(volume, clusterPos, 'y', true, 3, 2, 3, fruitColor)
	end
end

-- Create the crown — a bulge at the top of trunk where fronds emerge
local function createCrownBulge(volume, topPos, trunkStrength, leavesColor, variety)
	local bulgeR = trunkStrength + 1
	local bulgeH = math.max(1, trunkStrength)
	if variety == 'royal' then
		-- Prominent green crownshaft
		g_shape.cylinder(volume, g_ivec3.new(topPos.x, topPos.y - 1, topPos.z), 'y', bulgeR, 3, leavesColor)
	elseif variety == 'date' then
		-- Dense crown base
		g_shape.dome(volume, topPos, 'y', false, bulgeR * 2, bulgeH + 1, bulgeR * 2, leavesColor)
	else
		g_shape.dome(volume, topPos, 'y', false, bulgeR * 2, bulgeH, bulgeR * 2, leavesColor)
	end
end

function main(node, region, color, variety, trunkHeight, trunkStrength, trunkCurve, trunkLean,
	fronds, frondLength, frondDroop, frondWidth, fruits, barkRings,
	trunkColor, barkColor, leavesColor, leavesColor2, fruitColor, seed)

	if seed == 0 then
		math.randomseed(os.time())
	else
		math.randomseed(seed)
	end

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Apply variety-specific defaults adjustments
	local actualDroop = frondDroop
	local actualFronds = fronds
	local actualFrondLen = frondLength
	if variety == 'coconut' then
		actualDroop = math.max(frondDroop, math.floor(frondLength * 0.5))
	elseif variety == 'royal' then
		actualFronds = math.max(fronds, 6)
	elseif variety == 'fan' then
		actualDroop = math.max(2, math.floor(frondDroop * 0.5))
	elseif variety == 'date' then
		actualFronds = math.max(fronds, 8)
		actualDroop = math.max(frondDroop, math.floor(frondLength * 0.4))
	end

	-- Create trunk
	local topPos, leanDir = createTrunk(volume, pos, trunkHeight, trunkStrength, trunkCurve, trunkLean, trunkColor, barkColor, barkRings, variety)

	-- Crown bulge
	createCrownBulge(volume, topPos, trunkStrength, leavesColor, variety)

	-- Fruit cluster
	if fruits then
		createFruits(volume, topPos, actualFronds, fruitColor, variety)
	end

	-- Create fronds radiating from the crown
	-- Multiple tiers for a fuller look
	local tiers
	if variety == 'date' then
		tiers = 3
	elseif variety == 'royal' or variety == 'coconut' then
		tiers = 2
	else
		tiers = 1
	end

	for tier = 1, tiers do
		local tierFronds = actualFronds
		local tierLen
		local tierDroop

		if tier == 1 then
			-- Upper tier — shorter, more upright
			tierLen = math.max(3, actualFrondLen - math.random(1, 3))
			tierDroop = math.max(1, actualDroop - math.random(1, 3))
		elseif tier == 2 then
			-- Middle tier — standard
			tierLen = actualFrondLen
			tierDroop = actualDroop
		else
			-- Lower tier — longer, droopier (older fronds)
			tierLen = actualFrondLen + math.random(0, 2)
			tierDroop = actualDroop + math.random(1, 3)
			tierFronds = math.max(3, tierFronds - math.random(1, 3))
		end

		local tierOriginY = topPos.y + (tiers - tier)
		local tierOrigin = g_ivec3.new(topPos.x, tierOriginY, topPos.z)

		local angleStep = (2 * math.pi) / tierFronds
		local startAngle = math.random() * 2 * math.pi
		-- Stagger tiers so fronds don't overlap
		if tier > 1 then
			startAngle = startAngle + angleStep * 0.5
		end

		for i = 1, tierFronds do
			local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.4
			local thisLen = tierLen + math.random(-1, 2)
			local thisDroop = tierDroop + math.random(-1, 1)
			thisLen = math.max(3, thisLen)
			thisDroop = math.max(1, thisDroop)

			createFrond(volume, tierOrigin, angle, thisLen, thisDroop, frondWidth, leavesColor, leavesColor2, variety)
		end
	end

	-- Add a few dead/dry hanging fronds for realism on coconut and date
	if (variety == 'coconut' or variety == 'date') and tiers >= 2 then
		local deadFronds = math.random(1, 3)
		local deadOrigin = g_ivec3.new(topPos.x, topPos.y - 1, topPos.z)
		for _ = 1, deadFronds do
			local angle = math.random() * 2 * math.pi
			local ddx = math.cos(angle)
			local ddz = math.sin(angle)
			local deadEnd = g_ivec3.new(
				math.floor(deadOrigin.x + ddx * math.random(2, 4)),
				deadOrigin.y - math.random(3, 6),
				math.floor(deadOrigin.z + ddz * math.random(2, 4))
			)
			g_shape.line(volume, deadOrigin, deadEnd, barkColor, 1)
		end
	end
end
