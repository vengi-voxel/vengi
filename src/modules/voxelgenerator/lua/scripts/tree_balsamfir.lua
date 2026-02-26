--
-- Balsam Fir (Abies balsamea) tree generator
-- Creates a balsam fir with a straight slender trunk, narrow spire-shaped crown,
-- dense layered branch whorls with slight droop, and a pointed leader tip.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '24', min = '8', max = '50' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '2', min = '1', max = '4' },
		{ name = 'crownLayers', desc = 'Number of branch whorl layers', type = 'int', default = '8', min = '3', max = '16' },
		{ name = 'branchesPerWhorl', desc = 'Branches per whorl layer', type = 'int', default = '6', min = '3', max = '10' },
		{ name = 'baseWidth', desc = 'Width of the lowest branch layer', type = 'int', default = '10', min = '4', max = '20' },
		{ name = 'layerSpacing', desc = 'Vertical spacing between layers', type = 'int', default = '2', min = '1', max = '5' },
		{ name = 'branchDroop', desc = 'Downward droop at branch tips', type = 'int', default = '1', min = '0', max = '4' },
		{ name = 'tipHeight', desc = 'Height of the pointed leader tip', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'density', desc = 'Foliage density (fills between branches)', type = 'bool', default = 'true' },
		{ name = 'trunkColor', desc = 'Color of the trunk', type = 'colorindex', default = '1' },
		{ name = 'leavesColor', desc = 'Primary needle color (dark green)', type = 'colorindex', default = '2' },
		{ name = 'leavesColor2', desc = 'Secondary needle color (lighter green)', type = 'colorindex', default = '3' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a balsam fir with a narrow spire-shaped crown and dense layered branch whorls'
end

-- Create a single branch whorl layer at a given height with a given radius
local function createWhorl(volume, center, numBranches, radius, branchDroop, branchStrength, trunkColor, leavesColor, leavesColor2, dense)
	local angleStep = (2 * math.pi) / numBranches
	local startAngle = math.random() * 2 * math.pi

	for i = 1, numBranches do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.3
		local dx = math.cos(angle)
		local dz = math.sin(angle)

		-- Branch extends outward and droops slightly
		local tipX = math.floor(center.x + dx * radius)
		local tipZ = math.floor(center.z + dz * radius)
		local tipY = center.y - branchDroop - math.random(0, 1)

		local tipPos = g_ivec3.new(tipX, tipY, tipZ)
		g_shape.line(volume, center, tipPos, trunkColor, branchStrength)

		-- Foliage along the branch — small domes at mid and tip
		local midX = math.floor(center.x + dx * radius * 0.5)
		local midZ = math.floor(center.z + dz * radius * 0.5)
		local midY = center.y - math.floor(branchDroop * 0.4)
		local midPos = g_ivec3.new(midX, midY, midZ)

		-- Choose color with slight variation
		local col = leavesColor
		if math.random() > 0.65 then
			col = leavesColor2
		end

		-- Dense foliage at the tip
		local tipFoliageW = math.max(2, math.floor(radius * 0.45))
		local tipFoliageH = math.max(1, math.floor(tipFoliageW * 0.6))
		g_shape.dome(volume, tipPos, 'y', false, tipFoliageW * 2, tipFoliageH, tipFoliageW * 2, col)

		-- Foliage at midpoint
		local midFoliageW = math.max(1, math.floor(radius * 0.35))
		local midFoliageH = math.max(1, math.floor(midFoliageW * 0.5))
		g_shape.dome(volume, midPos, 'y', false, midFoliageW * 2, midFoliageH, midFoliageW * 2, leavesColor)
	end

	-- Fill the ring between branches with a flat dome for dense appearance
	if dense and radius >= 3 then
		local fillH = math.max(1, math.floor(branchDroop * 0.5) + 1)
		local fillCenter = g_ivec3.new(center.x, center.y - math.floor(branchDroop * 0.3), center.z)
		g_shape.dome(volume, fillCenter, 'y', false, radius * 2, fillH, radius * 2, leavesColor)
		-- Small inverted dome for underside volume
		g_shape.dome(volume, fillCenter, 'y', true, math.floor(radius * 1.4), math.max(1, fillH - 1), math.floor(radius * 1.4), leavesColor)
	end
end

function main(node, region, color, trunkHeight, trunkStrength, crownLayers, branchesPerWhorl,
	baseWidth, layerSpacing, branchDroop, tipHeight, density,
	trunkColor, leavesColor, leavesColor2, seed)

	if seed == 0 then
		math.randomseed(os.time())
	else
		math.randomseed(seed)
	end

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Straight slender trunk
	tree_utils.createTrunk(volume, pos, trunkHeight, trunkStrength, trunkColor)

	-- Pointed leader tip at the very top
	local topPos = g_ivec3.new(pos.x, pos.y + trunkHeight, pos.z)
	local tipTop = g_ivec3.new(pos.x, pos.y + trunkHeight + tipHeight, pos.z)
	g_shape.line(volume, topPos, tipTop, leavesColor, 1)
	-- Small cone at the tip for the spire
	g_shape.cone(volume, g_ivec3.new(topPos.x, topPos.y - 1, topPos.z), 'y', false, 3, tipHeight + 2, 3, leavesColor)

	-- Build whorl layers from top to bottom
	-- Each layer gets progressively wider, creating the narrow spire shape
	local crownStartY = pos.y + trunkHeight - 1
	local widthStep = (baseWidth - 2) / math.max(1, crownLayers - 1)

	for layer = 1, crownLayers do
		local layerY = crownStartY - (layer - 1) * layerSpacing
		local layerRadius = math.floor(2 + (layer - 1) * widthStep)
		layerRadius = math.max(2, layerRadius)

		local center = g_ivec3.new(pos.x, layerY, pos.z)
		local branchStr = 1
		if layer > crownLayers * 0.7 then
			-- Thicker branches near the bottom
			branchStr = math.min(2, trunkStrength - 1)
			branchStr = math.max(1, branchStr)
		end

		-- Slight droop increases toward lower layers
		local layerDroop = branchDroop + math.floor((layer - 1) * 0.3)

		createWhorl(volume, center, branchesPerWhorl, layerRadius, layerDroop,
			branchStr, trunkColor, leavesColor, leavesColor2, density)

		-- Rotate alternate whorls for a natural staggered look
		-- (handled inside createWhorl via random start angle)
	end

	-- Small bare trunk section below the lowest whorl for realism
	-- (already exists since trunk extends below crown)
end
