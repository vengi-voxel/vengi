--
-- Birch (Betula) tree generator
-- Creates a birch tree with a slender white trunk, delicate open crown,
-- fine arching branches with drooping tips, and a light airy canopy.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the main trunk', type = 'int', default = '18', min = '8', max = '40' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '2', min = '1', max = '4' },
		{ name = 'trunkCurve', desc = 'Horizontal curve of the trunk', type = 'int', default = '2', min = '0', max = '8' },
		{ name = 'secondaryTrunks', desc = 'Number of secondary trunks (0 for single)', type = 'int', default = '0', min = '0', max = '3' },
		{ name = 'mainBranches', desc = 'Number of main branches', type = 'int', default = '8', min = '3', max = '14' },
		{ name = 'branchLength', desc = 'Length of main branches', type = 'int', default = '7', min = '3', max = '16' },
		{ name = 'droopLength', desc = 'Length of drooping branch tips', type = 'int', default = '4', min = '1', max = '10' },
		{ name = 'foliageSize', desc = 'Base size of foliage clusters', type = 'int', default = '4', min = '2', max = '8' },
		{ name = 'crownWidth', desc = 'Width of the overall crown', type = 'int', default = '14', min = '6', max = '30' },
		{ name = 'crownHeight', desc = 'Height of the overall crown', type = 'int', default = '12', min = '4', max = '24' },
		{ name = 'trunkColor', desc = 'Color of the trunk (white bark)', type = 'hexcolor', default = '#D2C5A0' },
		{ name = 'leavesColor', desc = 'Primary foliage color', type = 'hexcolor', default = '#2E6B14' },
		{ name = 'leavesColor2', desc = 'Secondary foliage color (lighter)', type = 'hexcolor', default = '#4A8F32' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a birch tree with a slender white trunk, arching branches and airy canopy'
end

local drawBezier = tree_utils.drawBezier

-- Create a single trunk with gentle curve, returning top position
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
	-- Birch trunks stay slender — minimal taper
	local topThickness = math.max(1, trunkStrength - 1)
	drawBezier(volume, basePos, topPos, ctrl, trunkStrength, topThickness, trunkHeight, voxelColor)
	return topPos, topThickness
end

-- Create an arching branch with a drooping tip
local function createArchingBranch(volume, origin, angle, length, droopLen, trunkColor, leavesColor, leavesColor2, foliageSize)
	local dx = math.cos(angle)
	local dz = math.sin(angle)

	-- Branch arches upward and outward
	local rise = math.random(1, math.max(2, math.floor(length * 0.4)))
	local branchEnd = g_ivec3.new(
		math.floor(origin.x + dx * length),
		origin.y + rise,
		math.floor(origin.z + dz * length)
	)
	local archCtrl = g_ivec3.new(
		math.floor(origin.x + dx * length * 0.5),
		origin.y + rise + math.random(1, 3),
		math.floor(origin.z + dz * length * 0.5)
	)
	local archTip = drawBezier(volume, origin, branchEnd, archCtrl, 1, 1, math.max(5, length), trunkColor)

	-- Drooping tip from the arch end
	local droopEnd = g_ivec3.new(
		math.floor(archTip.x + dx * math.random(1, 2)),
		archTip.y - droopLen,
		math.floor(archTip.z + dz * math.random(1, 2))
	)
	local droopCtrl = g_ivec3.new(
		math.floor(archTip.x + dx * 1),
		archTip.y - math.floor(droopLen * 0.3),
		math.floor(archTip.z + dz * 1)
	)
	drawBezier(volume, archTip, droopEnd, droopCtrl, 1, 1, math.max(3, droopLen), trunkColor)

	-- Small foliage clusters along the branch
	local col = leavesColor
	if math.random() > 0.5 then
		col = leavesColor2
	end

	-- Foliage near the arch peak
	local peakCenter = g_ivec3.new(
		math.floor((origin.x + archTip.x) / 2),
		archTip.y + 1,
		math.floor((origin.z + archTip.z) / 2)
	)
	local fW = foliageSize + math.random(-1, 1)
	local fH = math.max(2, foliageSize - math.random(0, 1))
	fW = math.max(2, fW)
	g_shape.dome(volume, peakCenter, 'y', false, fW * 2, fH, fW * 2, col)

	-- Foliage at the arch tip
	local tipW = math.max(2, foliageSize - 1 + math.random(-1, 1))
	local tipH = math.max(1, tipW - 1)
	g_shape.dome(volume, archTip, 'y', false, tipW * 2, tipH, tipW * 2, leavesColor)

	-- Small foliage along the droop
	if droopLen >= 3 then
		local midDroop = g_ivec3.new(
			math.floor((archTip.x + droopEnd.x) / 2),
			math.floor((archTip.y + droopEnd.y) / 2),
			math.floor((archTip.z + droopEnd.z) / 2)
		)
		local dW = math.max(1, foliageSize - 2)
		g_shape.dome(volume, midDroop, 'y', false, dW * 2, math.max(1, dW), dW * 2, leavesColor2)
	end
end

-- Create the open, airy crown canopy
local function createCrown(volume, crownCenter, crownWidth, crownHeight, leavesColor, leavesColor2)
	-- Light, open ellipsoid — birch crowns are airy, not dense
	local halfW = math.floor(crownWidth / 2)

	-- Scatter small domes rather than one solid shape
	local numClusters = math.random(6, 10)
	for _ = 1, numClusters do
		local offX = math.random(-halfW + 2, halfW - 2)
		local offY = math.random(-math.floor(crownHeight * 0.3), math.floor(crownHeight * 0.5))
		local offZ = math.random(-halfW + 2, halfW - 2)
		local cCenter = g_ivec3.new(crownCenter.x + offX, crownCenter.y + offY, crownCenter.z + offZ)
		local cR = math.random(2, math.max(3, math.floor(crownWidth * 0.2)))
		local cH = math.max(1, cR - math.random(0, 1))
		local col = leavesColor
		if math.random() > 0.55 then
			col = leavesColor2
		end
		g_shape.dome(volume, cCenter, 'y', false, cR * 2, cH, cR * 2, col)
	end
end

function main(node, region, color, trunkHeight, trunkStrength, trunkCurve, secondaryTrunks,
	mainBranches, branchLength, droopLength, foliageSize, crownWidth, crownHeight,
	trunkColor, leavesColor, leavesColor2, seed)

	tree_utils.initSeed(seed)

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Main trunk
	local trunkTop, _ = createTrunk(volume, pos, trunkHeight, trunkStrength, trunkCurve, trunkColor)

	-- Collect all trunk tops for branch placement
	local trunkTops = { trunkTop }

	-- Optional secondary trunks (birches sometimes grow in clumps)
	for s = 1, secondaryTrunks do
		local sAngle = (s / secondaryTrunks) * 2 * math.pi + math.random() * 0.5
		local sDist = math.random(1, 3)
		local sBase = g_ivec3.new(
			pos.x + math.floor(math.cos(sAngle) * sDist),
			pos.y,
			pos.z + math.floor(math.sin(sAngle) * sDist)
		)
		local sHeight = trunkHeight - math.random(2, 4)
		local sCurve = trunkCurve + math.random(-1, 1)
		local sTop = createTrunk(volume, sBase, math.max(6, sHeight), math.max(1, trunkStrength - 1), math.max(0, sCurve), trunkColor)
		table.insert(trunkTops, sTop)
	end

	-- Create branches from each trunk top
	for _, top in ipairs(trunkTops) do
		local numBranches = mainBranches
		if top ~= trunkTop then
			numBranches = math.max(3, mainBranches - math.random(1, 3))
		end

		local angleStep = (2 * math.pi) / numBranches
		local startAngle = math.random() * 2 * math.pi

		for i = 1, numBranches do
			local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.5

			-- Branches originate from the upper portion of the trunk
			local branchT = 0.6 + 0.4 * ((i - 1) / math.max(1, numBranches - 1))
			local branchOrigin = g_ivec3.new(
				math.floor((1 - branchT) * pos.x + branchT * top.x),
				math.floor((1 - branchT) * pos.y + branchT * top.y),
				math.floor((1 - branchT) * pos.z + branchT * top.z)
			)

			local thisLength = branchLength + math.random(-2, 2)
			thisLength = math.max(3, thisLength)
			local thisDroop = droopLength + math.random(-1, 2)
			thisDroop = math.max(1, thisDroop)

			createArchingBranch(volume, branchOrigin, angle, thisLength, thisDroop,
				trunkColor, leavesColor, leavesColor2, foliageSize)
		end

		-- Light crown around this trunk top
		local cCenter = g_ivec3.new(top.x, top.y + math.floor(crownHeight / 5), top.z)
		local cW = crownWidth
		local cH = crownHeight
		if top ~= trunkTop then
			cW = math.max(6, crownWidth - math.random(2, 4))
			cH = math.max(4, crownHeight - math.random(2, 4))
		end
		createCrown(volume, cCenter, cW, cH, leavesColor, leavesColor2)
	end
end
