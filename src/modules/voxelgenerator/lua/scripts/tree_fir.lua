--
-- Fir tree generator
-- Creates a fir tree with a dense symmetrical conical crown, straight trunk,
-- tiered branch whorls with filled needle layers, and a pointed spire top.
-- Firs differ from pines: denser crown, more regular tiers, upward-angled needles.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '24', min = '8', max = '50' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '2', min = '1', max = '5' },
		{ name = 'crownLayers', desc = 'Number of branch tier layers', type = 'int', default = '8', min = '3', max = '16' },
		{ name = 'branchesPerWhorl', desc = 'Branches per whorl', type = 'int', default = '6', min = '3', max = '10' },
		{ name = 'baseWidth', desc = 'Width of the lowest branch tier', type = 'int', default = '10', min = '4', max = '20' },
		{ name = 'tipWidth', desc = 'Width of the topmost tier', type = 'int', default = '2', min = '1', max = '5' },
		{ name = 'layerSpacing', desc = 'Vertical spacing between tiers', type = 'int', default = '2', min = '1', max = '5' },
		{ name = 'branchUplift', desc = 'Upward angle of branches (fir-like)', type = 'int', default = '1', min = '0', max = '4' },
		{ name = 'density', desc = 'Fill between branches for dense look', type = 'bool', default = 'true' },
		{ name = 'tipHeight', desc = 'Height of the pointed spire tip', type = 'int', default = '4', min = '1', max = '8' },
		{ name = 'trunkColor', desc = 'Color of the trunk', type = 'hexcolor', default = '#8B4513' },
		{ name = 'leavesColor', desc = 'Primary needle color', type = 'hexcolor', default = '#1B4D1B' },
		{ name = 'leavesColor2', desc = 'Secondary needle color (lighter)', type = 'hexcolor', default = '#2E7D32' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a fir tree with a dense symmetrical conical crown, tiered branch whorls and pointed spire'
end

-- Create a single whorl layer of branches with foliage fill
local function createWhorl(volume, center, numBranches, radius, branchUplift, trunkColor, leavesColor, leavesColor2, dense)
	local angleStep = (2 * math.pi) / numBranches
	local startAngle = math.random() * 2 * math.pi

	for i = 1, numBranches do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.25
		local dx = math.cos(angle)
		local dz = math.sin(angle)

		-- Fir branches angle slightly upward (unlike pine/spruce which droop)
		local rise = branchUplift + math.random(0, 1)
		local tipX = math.floor(center.x + dx * radius)
		local tipZ = math.floor(center.z + dz * radius)
		local tipY = center.y + rise

		-- Branch line from trunk to tip
		local tipPos = g_ivec3.new(tipX, tipY, tipZ)
		g_shape.line(volume, center, tipPos, trunkColor, 1)

		-- Needle cluster at the tip
		local col = leavesColor
		if math.random() > 0.65 then
			col = leavesColor2
		end
		local tipW = math.max(2, math.floor(radius * 0.4) + math.random(-1, 1))
		local tipH = math.max(1, math.floor(tipW * 0.6))
		g_shape.dome(volume, tipPos, 'y', false, tipW * 2, tipH, tipW * 2, col)

		-- Needle cluster at midpoint for fuller look
		if radius >= 4 then
			local midPos = g_ivec3.new(
				math.floor(center.x + dx * radius * 0.5),
				center.y + math.floor(rise * 0.5),
				math.floor(center.z + dz * radius * 0.5)
			)
			local midW = math.max(1, math.floor(radius * 0.3))
			local midH = math.max(1, midW - 1)
			g_shape.dome(volume, midPos, 'y', false, midW * 2, midH, midW * 2, leavesColor)
		end
	end

	-- Dense fill between branches — flat ring of foliage
	if dense and radius >= 3 then
		local fillH = math.max(1, branchUplift + 1)
		local fillCenter = g_ivec3.new(center.x, center.y + math.floor(branchUplift * 0.3), center.z)
		g_shape.dome(volume, fillCenter, 'y', false, radius * 2, fillH, radius * 2, leavesColor)
		-- Underside fill for volume
		g_shape.dome(volume, center, 'y', true, math.floor(radius * 1.3), math.max(1, fillH - 1), math.floor(radius * 1.3), leavesColor)
	end
end

function main(node, region, color, trunkHeight, trunkStrength, crownLayers, branchesPerWhorl,
	baseWidth, tipWidth, layerSpacing, branchUplift, density, tipHeight,
	trunkColor, leavesColor, leavesColor2, seed)

	tree_utils.initSeed(seed)

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Straight trunk — firs are characteristically very straight
	tree_utils.createTrunk(volume, pos, trunkHeight, trunkStrength, trunkColor)

	-- Small base flare
	tree_utils.createBaseFlare(volume, pos, trunkStrength + 1, math.max(1, trunkStrength - 1), trunkColor)

	-- Pointed spire tip
	local topPos = g_ivec3.new(pos.x, pos.y + trunkHeight, pos.z)
	local tipTop = tree_utils.createSpireTip(volume, topPos, tipHeight, leavesColor)
	-- Secondary color accent at very tip
	g_shape.line(volume, g_ivec3.new(topPos.x, topPos.y + tipHeight - 1, topPos.z), tipTop, leavesColor2, 1)

	-- Crown starts near the top, just below the tip
	local crownTopY = topPos.y - 1
	local widthStep = (baseWidth - tipWidth) / math.max(1, crownLayers - 1)

	for layer = 1, crownLayers do
		local layerY = crownTopY - (layer - 1) * layerSpacing
		if layerY <= pos.y then
			break
		end

		local layerRadius = math.floor(tipWidth + (layer - 1) * widthStep)
		layerRadius = math.max(2, layerRadius)

		local center = g_ivec3.new(pos.x, layerY, pos.z)

		-- Uplift decreases slightly for lower tiers (older branches flatten)
		local layerUplift = math.max(0, branchUplift - math.floor((layer - 1) * 0.2))

		createWhorl(volume, center, branchesPerWhorl, layerRadius, layerUplift,
			trunkColor, leavesColor, leavesColor2, density)
	end

	-- Optional: short bare trunk section is implicit since the crown doesn't extend
	-- all the way down to the base on taller trees
end
