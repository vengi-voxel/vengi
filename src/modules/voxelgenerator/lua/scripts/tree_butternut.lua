--
-- Butternut (Juglans cinerea) tree generator
-- Creates a butternut tree with a short stout trunk, broad open flat-topped crown,
-- heavy spreading branches, and a sparse airy canopy of compound foliage.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the main trunk', type = 'int', default = '12', min = '5', max = '30' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk base', type = 'int', default = '4', min = '2', max = '8' },
		{ name = 'trunkCurve', desc = 'Horizontal curve offset of the trunk', type = 'int', default = '1', min = '0', max = '6' },
		{ name = 'crownWidth', desc = 'Width of the overall crown', type = 'int', default = '22', min = '8', max = '40' },
		{ name = 'crownHeight', desc = 'Height of the overall crown', type = 'int', default = '10', min = '3', max = '20' },
		{ name = 'mainBranches', desc = 'Number of main branches', type = 'int', default = '7', min = '3', max = '14' },
		{ name = 'branchLength', desc = 'Length of main branches', type = 'int', default = '10', min = '4', max = '20' },
		{ name = 'foliageClusters', desc = 'Foliage clusters scattered in the crown', type = 'int', default = '10', min = '3', max = '24' },
		{ name = 'foliageSize', desc = 'Base size of foliage clusters', type = 'int', default = '4', min = '2', max = '10' },
		{ name = 'roots', desc = 'Number of visible surface roots', type = 'int', default = '4', min = '0', max = '8' },
		{ name = 'rootLength', desc = 'Length of surface roots', type = 'int', default = '5', min = '2', max = '10' },
		{ name = 'trunkColor', desc = 'Color of the trunk and branches', type = 'hexcolor', default = '#8B4513' },
		{ name = 'leavesColor', desc = 'Primary foliage color', type = 'hexcolor', default = '#2E6B14' },
		{ name = 'leavesColor2', desc = 'Secondary foliage color (lighter)', type = 'hexcolor', default = '#4A8F32' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a butternut tree with a short stout trunk and broad flat-topped open crown'
end

local drawBezier = tree_utils.drawBezier

-- Create a heavy spreading branch, returning the tip position
local function createBranch(volume, origin, angle, length, thickness, voxelColor)
	local dx = math.cos(angle)
	local dz = math.sin(angle)

	-- Butternut branches spread wide and nearly horizontal, with a slight droop or rise
	local rise = math.random(-1, math.max(1, math.floor(length * 0.2)))
	local branchEnd = g_ivec3.new(
		math.floor(origin.x + dx * length),
		origin.y + rise,
		math.floor(origin.z + dz * length)
	)
	-- Control point slightly above for a gentle arch
	local ctrl = g_ivec3.new(
		math.floor(origin.x + dx * length * 0.5),
		origin.y + math.max(1, math.floor(length * 0.15)) + math.random(0, 2),
		math.floor(origin.z + dz * length * 0.5)
	)
	local tip = drawBezier(volume, origin, branchEnd, ctrl, thickness, 1, math.max(6, length), voxelColor)
	return tip
end

-- Create the broad, flat-topped open crown
local function createCrown(volume, crownCenter, crownWidth, crownHeight, numClusters, foliageSize, primaryColor, secondaryColor)
	-- Flat-topped dome for the canopy silhouette
	g_shape.dome(volume, g_ivec3.new(crownCenter.x, crownCenter.y - math.floor(crownHeight / 4), crownCenter.z),
		'y', false, crownWidth, crownHeight, crownWidth, primaryColor)

	-- Scatter additional clusters for an open, irregular canopy
	local halfW = math.floor(crownWidth / 2)
	local halfH = math.floor(crownHeight / 2)
	for _ = 1, numClusters do
		local offX = math.random(-halfW + 2, halfW - 2)
		local offY = math.random(-math.floor(halfH * 0.2), math.floor(halfH * 0.5))
		local offZ = math.random(-halfW + 2, halfW - 2)
		local clusterCenter = g_ivec3.new(crownCenter.x + offX, crownCenter.y + offY, crownCenter.z + offZ)
		local cRadius = foliageSize + math.random(-1, 2)
		local cHeight = math.max(2, foliageSize - math.random(0, 2))
		local col = primaryColor
		if math.random() > 0.6 then
			col = secondaryColor
		end
		g_shape.dome(volume, clusterCenter, 'y', false, cRadius * 2, cHeight, cRadius * 2, col)
	end

	-- Thin out parts of the crown with small air pockets for the open look
	local airPockets = math.floor(numClusters * 0.4)
	for _ = 1, airPockets do
		local offX = math.random(-halfW + 3, halfW - 3)
		local offY = math.random(0, math.floor(halfH * 0.4))
		local offZ = math.random(-halfW + 3, halfW - 3)
		local airCenter = g_ivec3.new(crownCenter.x + offX, crownCenter.y + offY, crownCenter.z + offZ)
		local airR = math.random(1, math.max(2, foliageSize - 2))
		g_shape.dome(volume, airCenter, 'y', false, airR * 2, math.max(1, airR - 1), airR * 2, -1)
	end
end

function main(node, region, color, trunkHeight, trunkStrength, trunkCurve, crownWidth, crownHeight,
	mainBranches, branchLength, foliageClusters, foliageSize, roots, rootLength,
	trunkColor, leavesColor, leavesColor2, seed)

	tree_utils.initSeed(seed)

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Root flare
	tree_utils.createBaseFlare(volume, pos, trunkStrength + 2, math.max(2, math.floor(trunkStrength * 0.7)), trunkColor)
	tree_utils.createBezierRoots(volume, pos, roots, rootLength, math.max(1, math.floor(trunkStrength * 0.5)), trunkColor)

	-- Stout trunk — butternut trunks stay fairly thick (slow taper)
	local topThickness = math.max(2, math.floor(trunkStrength * 0.6))
	local trunkTop = tree_utils.createCurvedTrunk(volume, pos, trunkHeight, trunkStrength, trunkCurve, topThickness, trunkColor, 0.3)

	-- Main branches — spread wide from upper trunk, some from lower for the open habit
	local angleStep = (2 * math.pi) / mainBranches
	local startAngle = math.random() * 2 * math.pi

	for i = 1, mainBranches do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.5

		-- Distribute branches along the upper 50% of the trunk
		local branchT = 0.5 + 0.5 * ((i - 1) / math.max(1, mainBranches - 1))
		local branchOrigin = g_ivec3.new(
			math.floor((1 - branchT) * pos.x + branchT * trunkTop.x),
			math.floor((1 - branchT) * pos.y + branchT * trunkTop.y),
			math.floor((1 - branchT) * pos.z + branchT * trunkTop.z)
		)

		-- Heavy branches, varied lengths
		local thisLength = branchLength + math.random(-2, 3)
		thisLength = math.max(4, thisLength)
		local branchThickness = math.max(1, math.floor(topThickness * 0.7))

		local tip = createBranch(volume, branchOrigin, angle, thisLength, branchThickness, trunkColor)

		-- Foliage cluster at the tip
		local cRadius = foliageSize + math.random(-1, 1)
		local cHeight = math.max(2, foliageSize - math.random(0, 1))
		cRadius = math.max(2, cRadius)
		tree_utils.dualColorFoliage(volume, tip, cRadius, cHeight, leavesColor, leavesColor2)

		-- Sub-branches for complexity
		if math.random() > 0.35 then
			local subAngle = angle + (math.random() - 0.5) * 1.4
			local subLength = math.max(3, math.floor(thisLength * 0.55))
			local subOrigin = g_ivec3.new(
				math.floor((branchOrigin.x + tip.x) / 2),
				math.floor((branchOrigin.y + tip.y) / 2),
				math.floor((branchOrigin.z + tip.z) / 2)
			)
			local subTip = createBranch(volume, subOrigin, subAngle, subLength, 1, trunkColor)
			local subR = math.max(2, foliageSize - 1)
			tree_utils.dualColorFoliage(volume, subTip, subR, math.max(2, subR - 1), leavesColor, leavesColor2)
		end

		-- Occasional drooping secondary sub-branch
		if math.random() > 0.6 then
			local droopAngle = angle + (math.random() - 0.5) * 1.0
			local droopLen = math.max(2, math.floor(thisLength * 0.4))
			local droopDx = math.cos(droopAngle)
			local droopDz = math.sin(droopAngle)
			local droopEnd = g_ivec3.new(
				math.floor(tip.x + droopDx * droopLen),
				tip.y - math.random(1, 3),
				math.floor(tip.z + droopDz * droopLen)
			)
			local droopCtrl = g_ivec3.new(
				math.floor(tip.x + droopDx * droopLen * 0.5),
				tip.y + 1,
				math.floor(tip.z + droopDz * droopLen * 0.5)
			)
			drawBezier(volume, tip, droopEnd, droopCtrl, 1, 1, math.max(4, droopLen), trunkColor)
			local droopR = math.max(2, foliageSize - 2)
			tree_utils.dualColorFoliage(volume, droopEnd, droopR, math.max(1, droopR - 1), leavesColor, leavesColor2)
		end
	end

	-- Broad flat-topped crown
	local crownCenter = g_ivec3.new(trunkTop.x, trunkTop.y + math.floor(crownHeight / 4), trunkTop.z)
	createCrown(volume, crownCenter, crownWidth, crownHeight, foliageClusters, foliageSize, leavesColor, leavesColor2)
end
