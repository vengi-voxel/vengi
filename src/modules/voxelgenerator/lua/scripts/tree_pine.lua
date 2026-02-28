--
-- Pine tree generator
-- Creates a pine tree with an irregular conical crown, exposed lower trunk,
-- layered branch whorls with needle clusters, and a natural asymmetric look.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '22', min = '8', max = '50' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk base', type = 'int', default = '2', min = '1', max = '5' },
		{ name = 'trunkCurve', desc = 'Slight trunk curve', type = 'int', default = '1', min = '0', max = '6' },
		{ name = 'crownStart', desc = 'Height ratio where crown begins (% of trunk)', type = 'int', default = '40', min = '20', max = '70' },
		{ name = 'crownLayers', desc = 'Number of branch whorl layers', type = 'int', default = '6', min = '3', max = '14' },
		{ name = 'branchesPerWhorl', desc = 'Branches per whorl', type = 'int', default = '5', min = '3', max = '8' },
		{ name = 'baseRadius', desc = 'Radius of the lowest branch layer', type = 'int', default = '8', min = '3', max = '18' },
		{ name = 'topRadius', desc = 'Radius of the topmost layer', type = 'int', default = '2', min = '1', max = '6' },
		{ name = 'branchDroop', desc = 'Downward droop of branches', type = 'int', default = '2', min = '0', max = '6' },
		{ name = 'needleClusterSize', desc = 'Size of needle clusters at branch tips', type = 'int', default = '4', min = '2', max = '8' },
		{ name = 'irregularity', desc = 'Crown irregularity (0=symmetric, 5=very irregular)', type = 'int', default = '2', min = '0', max = '5' },
		{ name = 'trunkColor', desc = 'Color of the trunk', type = 'hexcolor', default = '#8B4513' },
		{ name = 'leavesColor', desc = 'Primary needle color', type = 'hexcolor', default = '#1B4D1B' },
		{ name = 'leavesColor2', desc = 'Secondary needle color', type = 'hexcolor', default = '#2E7D32' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a pine tree with an irregular conical crown, layered branch whorls and needle clusters'
end

local drawBezier = tree_utils.drawBezier

-- Create a needle cluster at a position — irregular dome with slight randomness
local function createNeedleCluster(volume, center, size, primaryColor, secondaryColor)
	local w = size + math.random(-1, 1)
	local h = math.max(1, size - math.random(0, 2))
	w = math.max(2, w)
	local col = primaryColor
	if math.random() > 0.6 then
		col = secondaryColor
	end
	g_shape.dome(volume, center, 'y', false, w * 2, h, w * 2, col)
	-- Small underside fill
	if size >= 3 then
		local underH = math.max(1, math.floor(h * 0.4))
		g_shape.dome(volume, center, 'y', true, math.floor(w * 1.2), underH, math.floor(w * 1.2), primaryColor)
	end
end

-- Create a single branch with a curve, needle clusters along it and at the tip
local function createBranch(volume, origin, angle, length, droop, branchThickness, clusterSize, trunkColor, leavesColor, leavesColor2, irregularity)
	local dx = math.cos(angle)
	local dz = math.sin(angle)

	-- Pine branches extend outward with a slight upward arch then droop at the tip
	local archHeight = math.random(0, math.max(1, math.floor(length * 0.2)))
	local tipDroop = droop + math.random(0, irregularity)

	local branchEnd = g_ivec3.new(
		math.floor(origin.x + dx * length),
		origin.y + archHeight - tipDroop,
		math.floor(origin.z + dz * length)
	)
	local ctrl = g_ivec3.new(
		math.floor(origin.x + dx * length * 0.45),
		origin.y + archHeight + 1,
		math.floor(origin.z + dz * length * 0.45)
	)
	local tip = drawBezier(volume, origin, branchEnd, ctrl, branchThickness, 1, math.max(5, length), trunkColor)

	-- Needle cluster at the tip
	createNeedleCluster(volume, tip, clusterSize, leavesColor, leavesColor2)

	-- Needle cluster at midpoint
	if length >= 5 then
		local midT = 0.5
		local invT = 0.5
		local midPos = g_ivec3.new(
			math.floor(invT * invT * origin.x + 2 * invT * midT * ctrl.x + midT * midT * branchEnd.x),
			math.floor(invT * invT * origin.y + 2 * invT * midT * ctrl.y + midT * midT * branchEnd.y),
			math.floor(invT * invT * origin.z + 2 * invT * midT * ctrl.z + midT * midT * branchEnd.z)
		)
		local midSize = math.max(2, clusterSize - 1)
		createNeedleCluster(volume, midPos, midSize, leavesColor, leavesColor2)
	end

	-- Occasional secondary sub-branch
	if length >= 6 and math.random() > 0.5 then
		local subT = 0.4 + math.random() * 0.2
		local invT = 1.0 - subT
		local subOrigin = g_ivec3.new(
			math.floor(invT * invT * origin.x + 2 * invT * subT * ctrl.x + subT * subT * branchEnd.x),
			math.floor(invT * invT * origin.y + 2 * invT * subT * ctrl.y + subT * subT * branchEnd.y),
			math.floor(invT * invT * origin.z + 2 * invT * subT * ctrl.z + subT * subT * branchEnd.z)
		)
		local subAngle = angle + (math.random() - 0.5) * 1.2
		local subDx = math.cos(subAngle)
		local subDz = math.sin(subAngle)
		local subLen = math.max(2, math.floor(length * 0.4))
		local subEnd = g_ivec3.new(
			math.floor(subOrigin.x + subDx * subLen),
			subOrigin.y - math.random(1, 2),
			math.floor(subOrigin.z + subDz * subLen)
		)
		g_shape.line(volume, subOrigin, subEnd, trunkColor, 1)
		createNeedleCluster(volume, subEnd, math.max(2, clusterSize - 2), leavesColor, leavesColor2)
	end
end

function main(node, region, color, trunkHeight, trunkStrength, trunkCurve, crownStart,
	crownLayers, branchesPerWhorl, baseRadius, topRadius, branchDroop, needleClusterSize,
	irregularity, trunkColor, leavesColor, leavesColor2, seed)

	tree_utils.initSeed(seed)

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Create the trunk with a slight lean
	local curveDir = math.random() * 2 * math.pi
	local curveX = math.floor(math.cos(curveDir) * trunkCurve)
	local curveZ = math.floor(math.sin(curveDir) * trunkCurve)
	local topPos = g_ivec3.new(pos.x + curveX, pos.y + trunkHeight, pos.z + curveZ)
	local ctrl = g_ivec3.new(
		pos.x + math.floor(curveX * 0.3),
		pos.y + math.floor(trunkHeight * 0.5),
		pos.z + math.floor(curveZ * 0.3)
	)
	local topThickness = math.max(1, trunkStrength - 1)
	drawBezier(volume, pos, topPos, ctrl, trunkStrength, topThickness, trunkHeight, trunkColor)

	-- Small base flare
	g_shape.dome(volume, pos, 'y', false,
		(trunkStrength + 1) * 2, math.max(1, trunkStrength - 1), (trunkStrength + 1) * 2, trunkColor)

	-- Pointed tip at the very top
	local tipTop = g_ivec3.new(topPos.x, topPos.y + 3, topPos.z)
	g_shape.line(volume, topPos, tipTop, leavesColor, 1)
	g_shape.cone(volume, g_ivec3.new(topPos.x, topPos.y - 1, topPos.z), 'y', false, 3, 4, 3, leavesColor)

	-- Crown begins at crownStart% of the trunk height
	local crownBaseY = pos.y + math.floor(trunkHeight * crownStart / 100)
	local crownTopY = topPos.y - 1
	local layerSpacing = math.max(1, math.floor((crownTopY - crownBaseY) / math.max(1, crownLayers)))

	-- Build whorl layers from top to bottom
	for layer = 1, crownLayers do
		local layerT = (layer - 1) / math.max(1, crownLayers - 1) -- 0 at top, 1 at bottom
		local layerY = crownTopY - (layer - 1) * layerSpacing

		-- Interpolate radius from topRadius to baseRadius
		local layerRadius = math.floor(topRadius + (baseRadius - topRadius) * layerT)
		-- Add irregularity
		layerRadius = layerRadius + math.random(-irregularity, irregularity)
		layerRadius = math.max(2, layerRadius)

		-- Interpolate trunk X,Z position at this height
		local ht = (layerY - pos.y) / trunkHeight
		local invHt = 1.0 - ht
		local centerX = math.floor(invHt * invHt * pos.x + 2 * invHt * ht * ctrl.x + ht * ht * topPos.x)
		local centerZ = math.floor(invHt * invHt * pos.z + 2 * invHt * ht * ctrl.z + ht * ht * topPos.z)
		local center = g_ivec3.new(centerX, layerY, centerZ)

		-- Branch thickness decreases toward the top
		local branchThick = 1
		if layerT > 0.6 then
			branchThick = math.min(2, math.max(1, topThickness))
		end

		-- Droop increases slightly toward lower layers
		local layerDroop = branchDroop + math.floor(layerT * 2)

		-- Cluster size decreases toward top
		local layerCluster = math.max(2, needleClusterSize - math.floor((1 - layerT) * 2))

		local angleStep = (2 * math.pi) / branchesPerWhorl
		local startAngle = math.random() * 2 * math.pi
		-- Stagger alternating whorls
		if layer % 2 == 0 then
			startAngle = startAngle + angleStep * 0.5
		end

		for i = 1, branchesPerWhorl do
			local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.3
			local thisLen = layerRadius + math.random(-irregularity, irregularity)
			thisLen = math.max(2, thisLen)

			createBranch(volume, center, angle, thisLen, layerDroop, branchThick,
				layerCluster, trunkColor, leavesColor, leavesColor2, irregularity)
		end

		-- Fill ring between branches with a sparse dome for density
		if layerRadius >= 3 then
			local fillH = math.max(1, math.floor(layerDroop * 0.3) + 1)
			local fillCenter = g_ivec3.new(center.x, center.y - math.floor(layerDroop * 0.2), center.z)
			local fillCol = leavesColor
			if math.random() > 0.7 then
				fillCol = leavesColor2
			end
			g_shape.dome(volume, fillCenter, 'y', false, math.floor(layerRadius * 1.4), fillH, math.floor(layerRadius * 1.4), fillCol)
		end
	end

	-- Add a few dead lower branches (stubs) below the crown for realism
	local deadBranches = math.random(1, 3)
	for _ = 1, deadBranches do
		local deadY = crownBaseY - math.random(1, math.max(2, math.floor((crownBaseY - pos.y) * 0.3)))
		if deadY > pos.y + 2 then
			local deadAngle = math.random() * 2 * math.pi
			local deadDx = math.cos(deadAngle)
			local deadDz = math.sin(deadAngle)
			local deadLen = math.random(1, 3)
			local deadStart = g_ivec3.new(pos.x, deadY, pos.z)
			local deadEnd = g_ivec3.new(
				math.floor(pos.x + deadDx * deadLen),
				deadY - math.random(0, 1),
				math.floor(pos.z + deadDz * deadLen)
			)
			g_shape.line(volume, deadStart, deadEnd, trunkColor, 1)
		end
	end
end
