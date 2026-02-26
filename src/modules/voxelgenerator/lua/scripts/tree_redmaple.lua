--
-- Red Maple (Acer rubrum) tree generator
-- Creates a red maple with a slightly irregular trunk, broad rounded crown,
-- spreading branches with clusters of vivid red foliage, and a root flare.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the main trunk', type = 'int', default = '16', min = '6', max = '40' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk base', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'trunkCurve', desc = 'Horizontal curve offset of the trunk', type = 'int', default = '2', min = '0', max = '8' },
		{ name = 'crownWidth', desc = 'Width of the overall crown', type = 'int', default = '18', min = '6', max = '40' },
		{ name = 'crownHeight', desc = 'Height of the overall crown', type = 'int', default = '14', min = '4', max = '30' },
		{ name = 'mainBranches', desc = 'Number of main branches', type = 'int', default = '6', min = '3', max = '12' },
		{ name = 'branchLength', desc = 'Length of main branches', type = 'int', default = '8', min = '3', max = '18' },
		{ name = 'foliageClusters', desc = 'Extra foliage clusters in the crown', type = 'int', default = '8', min = '2', max = '20' },
		{ name = 'foliageSize', desc = 'Base size of foliage clusters', type = 'int', default = '5', min = '2', max = '10' },
		{ name = 'roots', desc = 'Number of visible surface roots', type = 'int', default = '4', min = '0', max = '8' },
		{ name = 'rootLength', desc = 'Length of surface roots', type = 'int', default = '4', min = '2', max = '10' },
		{ name = 'trunkColor', desc = 'Color of the trunk and branches', type = 'hexcolor', default = '#6B4226' },
		{ name = 'leavesColor', desc = 'Primary foliage color (red)', type = 'hexcolor', default = '#C62828' },
		{ name = 'leavesColor2', desc = 'Secondary foliage color (darker red)', type = 'hexcolor', default = '#8B1A1A' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a red maple tree with a broad rounded crown and vivid red foliage'
end

-- Draw a bezier curve between two points with tapering thickness, returning final position
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

-- Create root flare at the base of the trunk
local function createRootFlare(volume, basePos, numRoots, rootLength, trunkStrength, voxelColor)
	-- Bulge at the base
	local bulgeRadius = trunkStrength + 1
	g_shape.dome(volume, basePos, 'y', false,
		bulgeRadius * 2, math.max(2, math.floor(trunkStrength * 0.6)), bulgeRadius * 2, voxelColor)

	if numRoots <= 0 then
		return
	end
	local angleStep = (2 * math.pi) / numRoots
	local startAngle = math.random() * 2 * math.pi
	for i = 1, numRoots do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.5
		local dx = math.cos(angle)
		local dz = math.sin(angle)
		local startP = g_ivec3.new(basePos.x, basePos.y, basePos.z)
		local endP = g_ivec3.new(
			math.floor(basePos.x + dx * rootLength),
			basePos.y - math.random(0, 1),
			math.floor(basePos.z + dz * rootLength)
		)
		local ctrl = g_ivec3.new(
			math.floor(basePos.x + dx * rootLength * 0.5),
			basePos.y + 1,
			math.floor(basePos.z + dz * rootLength * 0.5)
		)
		local rootThickness = math.max(1, math.floor(trunkStrength * 0.5))
		drawBezier(volume, startP, endP, ctrl, rootThickness, 1, math.max(4, rootLength), voxelColor)
	end
end

-- Create the trunk with a slight curve and taper
local function createTrunk(volume, basePos, trunkHeight, trunkStrength, trunkCurve, voxelColor)
	local curveDir = math.random() * 2 * math.pi
	local curveX = math.floor(math.cos(curveDir) * trunkCurve)
	local curveZ = math.floor(math.sin(curveDir) * trunkCurve)

	local topPos = g_ivec3.new(
		basePos.x + curveX,
		basePos.y + trunkHeight,
		basePos.z + curveZ
	)
	local ctrl = g_ivec3.new(
		basePos.x + math.floor(curveX * 0.4),
		basePos.y + math.floor(trunkHeight * 0.5),
		basePos.z + math.floor(curveZ * 0.4)
	)
	local topThickness = math.max(1, math.floor(trunkStrength * 0.5))
	drawBezier(volume, basePos, topPos, ctrl, trunkStrength, topThickness, trunkHeight, voxelColor)
	return topPos, topThickness, curveDir
end

-- Create a spreading branch from the trunk, returning tip position
local function createBranch(volume, origin, angle, length, thickness, voxelColor)
	local dx = math.cos(angle)
	local dz = math.sin(angle)

	-- Red maples have branches that spread outward and slightly upward
	local rise = math.random(1, math.max(2, math.floor(length * 0.4)))
	local branchEnd = g_ivec3.new(
		math.floor(origin.x + dx * length),
		origin.y + rise,
		math.floor(origin.z + dz * length)
	)
	local ctrl = g_ivec3.new(
		math.floor(origin.x + dx * length * 0.5),
		origin.y + math.floor(rise * 0.6) + math.random(0, 2),
		math.floor(origin.z + dz * length * 0.5)
	)
	local tip = drawBezier(volume, origin, branchEnd, ctrl, thickness, 1, math.max(5, length), voxelColor)
	return tip
end

-- Create a foliage cluster with mixed red colors for the maple's vivid crown
local function createFoliageCluster(volume, center, radius, height, primaryColor, secondaryColor)
	-- Main dome
	g_shape.dome(volume, center, 'y', false, radius * 2, height, radius * 2, primaryColor)
	-- Overlay a slightly smaller dome with the secondary color for color variation
	local innerRadius = math.max(1, radius - 1)
	local innerHeight = math.max(1, height - 1)
	-- Offset the secondary color dome slightly for a natural mixed effect
	local offX = math.random(-1, 1)
	local offZ = math.random(-1, 1)
	local offCenter = g_ivec3.new(center.x + offX, center.y, center.z + offZ)
	g_shape.dome(volume, offCenter, 'y', false, innerRadius * 2, innerHeight, innerRadius * 2, secondaryColor)
	-- Small bottom fill for volume
	local underHeight = math.max(1, math.floor(height / 3))
	g_shape.dome(volume, center, 'y', true, math.floor(radius * 1.4), underHeight, math.floor(radius * 1.4), primaryColor)
end

-- Fill in the overall crown shape with scattered foliage clusters
local function createCrown(volume, crownCenter, crownWidth, crownHeight, numClusters, foliageSize, primaryColor, secondaryColor)
	-- Main crown ellipsoid
	g_shape.ellipse(volume, g_ivec3.new(crownCenter.x, crownCenter.y - math.floor(crownHeight / 3), crownCenter.z),
		'y', crownWidth, crownHeight, crownWidth, primaryColor)
	-- Scatter additional clusters across the crown for an irregular, natural canopy
	local halfW = math.floor(crownWidth / 2)
	local halfH = math.floor(crownHeight / 2)
	for _ = 1, numClusters do
		local offX = math.random(-halfW + 2, halfW - 2)
		local offY = math.random(-math.floor(halfH * 0.3), math.floor(halfH * 0.6))
		local offZ = math.random(-halfW + 2, halfW - 2)
		local clusterCenter = g_ivec3.new(crownCenter.x + offX, crownCenter.y + offY, crownCenter.z + offZ)
		local clusterRadius = foliageSize + math.random(-1, 2)
		local clusterHeight = math.max(2, foliageSize - math.random(0, 2))
		-- Alternate between primary and secondary colors
		local color = primaryColor
		if math.random() > 0.5 then
			color = secondaryColor
		end
		g_shape.dome(volume, clusterCenter, 'y', false, clusterRadius * 2, clusterHeight, clusterRadius * 2, color)
	end
end

function main(node, region, color, trunkHeight, trunkStrength, trunkCurve, crownWidth, crownHeight,
	mainBranches, branchLength, foliageClusters, foliageSize, roots, rootLength,
	trunkColor, leavesColor, leavesColor2, seed)

	if seed == 0 then
		math.randomseed(os.time())
	else
		math.randomseed(seed)
	end

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Create root flare
	createRootFlare(volume, pos, roots, rootLength, trunkStrength, trunkColor)

	-- Create trunk
	local trunkTop, topThickness, _ = createTrunk(volume, pos, trunkHeight, trunkStrength, trunkCurve, trunkColor)

	-- Create main branches radiating from the upper trunk
	local angleStep = (2 * math.pi) / mainBranches
	local startAngle = math.random() * 2 * math.pi

	for i = 1, mainBranches do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.5

		-- Branches originate from the upper 40% of the trunk
		local branchT = 0.6 + 0.4 * ((i - 1) / math.max(1, mainBranches - 1))
		-- Interpolate along the trunk curve
		local invT = 1.0 - branchT
		local branchOrigin = g_ivec3.new(
			math.floor(invT * pos.x + branchT * trunkTop.x),
			math.floor(invT * pos.y + branchT * trunkTop.y),
			math.floor(invT * pos.z + branchT * trunkTop.z)
		)

		local thisLength = branchLength + math.random(-2, 2)
		thisLength = math.max(3, thisLength)
		local branchThickness = math.max(1, math.floor(topThickness * 0.8))

		local tip = createBranch(volume, branchOrigin, angle, thisLength, branchThickness, trunkColor)

		-- Foliage cluster at the branch tip
		local clusterRadius = foliageSize + math.random(-1, 1)
		local clusterHeight = math.max(2, foliageSize - math.random(0, 1))
		clusterRadius = math.max(2, clusterRadius)
		createFoliageCluster(volume, tip, clusterRadius, clusterHeight, leavesColor, leavesColor2)

		-- Occasionally add a sub-branch
		if math.random() > 0.4 then
			local subAngle = angle + (math.random() - 0.5) * 1.5
			local subLength = math.max(2, math.floor(thisLength * 0.5))
			-- Sub-branch starts partway along the main branch
			local subOrigin = g_ivec3.new(
				math.floor((branchOrigin.x + tip.x) / 2),
				math.floor((branchOrigin.y + tip.y) / 2),
				math.floor((branchOrigin.z + tip.z) / 2)
			)
			local subTip = createBranch(volume, subOrigin, subAngle, subLength, 1, trunkColor)
			local subCluster = math.max(2, foliageSize - 1)
			createFoliageCluster(volume, subTip, subCluster, math.max(2, subCluster - 1), leavesColor, leavesColor2)
		end
	end

	-- Create the overall crown canopy
	local crownCenter = g_ivec3.new(trunkTop.x, trunkTop.y + math.floor(crownHeight / 4), trunkTop.z)
	createCrown(volume, crownCenter, crownWidth, crownHeight, foliageClusters, foliageSize, leavesColor, leavesColor2)
end
