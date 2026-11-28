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

local drawBezier = tree_utils.drawBezier

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

	tree_utils.initSeed(seed)

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Create root flare
	tree_utils.createBaseFlare(volume, pos, trunkStrength + 1, math.max(2, math.floor(trunkStrength * 0.6)), trunkColor)
	tree_utils.createBezierRoots(volume, pos, roots, rootLength, math.max(1, math.floor(trunkStrength * 0.5)), trunkColor)

	-- Create trunk
	local topThickness = math.max(1, math.floor(trunkStrength * 0.5))
	local trunkTop = tree_utils.createCurvedTrunk(volume, pos, trunkHeight, trunkStrength, trunkCurve, topThickness, trunkColor)

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
		tree_utils.dualColorFoliage(volume, tip, clusterRadius, clusterHeight, leavesColor, leavesColor2, true)

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
			tree_utils.dualColorFoliage(volume, subTip, subCluster, math.max(2, subCluster - 1), leavesColor, leavesColor2, true)
		end
	end

	-- Create the overall crown canopy
	local crownCenter = g_ivec3.new(trunkTop.x, trunkTop.y + math.floor(crownHeight / 4), trunkTop.z)
	createCrown(volume, crownCenter, crownWidth, crownHeight, foliageClusters, foliageSize, leavesColor, leavesColor2)
end
