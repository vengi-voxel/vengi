--
-- Black Willow (Salix nigra) tree generator
-- Creates a black willow with a thick irregular trunk, broad open crown,
-- arching branches with drooping tips, and cascading hanging foliage.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the main trunk', type = 'int', default = '16', min = '8', max = '40' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk base', type = 'int', default = '3', min = '2', max = '8' },
		{ name = 'crownSpread', desc = 'Horizontal spread of the crown', type = 'int', default = '14', min = '6', max = '30' },
		{ name = 'mainBranches', desc = 'Number of main arching branches', type = 'int', default = '7', min = '3', max = '12' },
		{ name = 'branchLength', desc = 'Length of main branches', type = 'int', default = '10', min = '4', max = '20' },
		{ name = 'droopLength', desc = 'Length of drooping branch tips', type = 'int', default = '8', min = '2', max = '16' },
		{ name = 'hangingVines', desc = 'Number of hanging foliage strands per branch', type = 'int', default = '5', min = '1', max = '10' },
		{ name = 'vineMinLength', desc = 'Min length of hanging foliage', type = 'int', default = '4', min = '2', max = '10' },
		{ name = 'vineMaxLength', desc = 'Max length of hanging foliage', type = 'int', default = '12', min = '4', max = '25' },
		{ name = 'forkChance', desc = 'Chance (%) that main trunk forks', type = 'int', default = '40', min = '0', max = '100' },
		{ name = 'trunkColor', desc = 'Color of the trunk and branches', type = 'hexcolor', default = '#5C4033' },
		{ name = 'leavesColor', desc = 'Color of the foliage', type = 'hexcolor', default = '#6B8E23' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a black willow tree with arching branches, drooping tips and cascading foliage'
end

local drawBezier = tree_utils.drawBezier

-- Create the root flare at the base of the trunk
local function createRootFlare(volume, basePos, trunkStrength, voxelColor)
	local numRoots = math.random(3, 6)
	local angleStep = (2 * math.pi) / numRoots
	local startAngle = math.random() * 2 * math.pi
	for i = 1, numRoots do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.4
		local dx = math.cos(angle)
		local dz = math.sin(angle)
		local rootLen = math.random(2, trunkStrength + 2)
		local startP = g_ivec3.new(basePos.x, basePos.y, basePos.z)
		local endP = g_ivec3.new(
			math.floor(basePos.x + dx * rootLen),
			basePos.y - math.random(0, 1),
			math.floor(basePos.z + dz * rootLen)
		)
		local ctrl = g_ivec3.new(
			math.floor(basePos.x + dx * rootLen * 0.5),
			basePos.y + 1,
			math.floor(basePos.z + dz * rootLen * 0.5)
		)
		local thickness = math.max(1, math.floor(trunkStrength * 0.5))
		drawBezier(volume, startP, endP, ctrl, thickness, 1, math.max(4, rootLen), voxelColor)
	end
	-- Bulge at base
	local bulgeRadius = trunkStrength + 1
	g_shape.dome(volume, basePos, 'y', false,
		bulgeRadius * 2, math.max(2, math.floor(trunkStrength * 0.6)), bulgeRadius * 2, voxelColor)
end

-- Build the main trunk with optional irregularity
local function createTrunk(volume, basePos, trunkHeight, trunkStrength, voxelColor)
	-- Slight lean for natural look
	local leanAngle = math.random() * 2 * math.pi
	local leanAmount = math.random(0, 2)
	local leanX = math.floor(math.cos(leanAngle) * leanAmount)
	local leanZ = math.floor(math.sin(leanAngle) * leanAmount)

	local topPos = g_ivec3.new(
		basePos.x + leanX,
		basePos.y + trunkHeight,
		basePos.z + leanZ
	)
	local ctrl = g_ivec3.new(
		basePos.x + math.floor(leanX * 0.3),
		basePos.y + math.floor(trunkHeight * 0.5),
		basePos.z + math.floor(leanZ * 0.3)
	)
	local topThickness = math.max(2, math.floor(trunkStrength * 0.6))
	drawBezier(volume, basePos, topPos, ctrl, trunkStrength, topThickness, trunkHeight, voxelColor)
	return topPos, topThickness
end

-- Build a forked secondary trunk
local function createFork(volume, forkPos, trunkHeight, trunkStrength, voxelColor)
	local forkAngle = math.random() * 2 * math.pi
	local forkSpread = math.random(3, 6)
	local forkHeight = math.floor(trunkHeight * (0.4 + math.random() * 0.3))
	local dx = math.floor(math.cos(forkAngle) * forkSpread)
	local dz = math.floor(math.sin(forkAngle) * forkSpread)
	local forkTop = g_ivec3.new(
		forkPos.x + dx,
		forkPos.y + forkHeight,
		forkPos.z + dz
	)
	local ctrl = g_ivec3.new(
		forkPos.x + math.floor(dx * 0.4),
		forkPos.y + math.floor(forkHeight * 0.6),
		forkPos.z + math.floor(dz * 0.4)
	)
	local topThick = math.max(1, math.floor(trunkStrength * 0.4))
	drawBezier(volume, forkPos, forkTop, ctrl, trunkStrength, topThick, forkHeight, voxelColor)
	return forkTop, topThick
end

-- Create a single arching branch with drooping tip
local function createArchingBranch(volume, origin, angle, branchLength, droopLength, trunkThickness, voxelColor, leavesColor, hangingVines, vineMinLength, vineMaxLength)
	local dx = math.cos(angle)
	local dz = math.sin(angle)

	-- Main branch arches upward and outward
	local archHeight = math.random(2, math.max(3, math.floor(branchLength * 0.5)))
	local branchEnd = g_ivec3.new(
		math.floor(origin.x + dx * branchLength),
		origin.y + archHeight,
		math.floor(origin.z + dz * branchLength)
	)
	local archCtrl = g_ivec3.new(
		math.floor(origin.x + dx * branchLength * 0.5),
		origin.y + archHeight + math.random(1, 3),
		math.floor(origin.z + dz * branchLength * 0.5)
	)
	local branchThickness = math.max(1, math.floor(trunkThickness * 0.6))
	local archTip = drawBezier(volume, origin, branchEnd, archCtrl, branchThickness, 1, math.max(6, branchLength), voxelColor)

	-- Drooping tip from the end of the arch
	local droopEnd = g_ivec3.new(
		math.floor(archTip.x + dx * math.random(1, 3)),
		archTip.y - droopLength,
		math.floor(archTip.z + dz * math.random(1, 3))
	)
	local droopCtrl = g_ivec3.new(
		math.floor(archTip.x + dx * 2),
		archTip.y - math.floor(droopLength * 0.3),
		math.floor(archTip.z + dz * 2)
	)
	drawBezier(volume, archTip, droopEnd, droopCtrl, 1, 1, math.max(4, droopLength), leavesColor)

	-- Small foliage cluster near the arch peak
	local foliageCenter = g_ivec3.new(
		math.floor((origin.x + archTip.x) / 2 + dx * 1),
		archTip.y + 1,
		math.floor((origin.z + archTip.z) / 2 + dz * 1)
	)
	local foliageW = math.random(3, 6)
	local foliageH = math.random(2, 4)
	g_shape.dome(volume, foliageCenter, 'y', false, foliageW * 2, foliageH, foliageW * 2, leavesColor)

	-- Foliage cluster at the branch tip
	local tipFoliageW = math.random(2, 4)
	local tipFoliageH = math.random(2, 3)
	g_shape.dome(volume, archTip, 'y', false, tipFoliageW * 2, tipFoliageH, tipFoliageW * 2, leavesColor)

	-- Hanging foliage strands along the branch
	createHangingFoliage(volume, origin, archTip, branchLength, dx, dz, hangingVines, vineMinLength, vineMaxLength, leavesColor)
end

-- Create hanging foliage strands (the signature cascading look)
function createHangingFoliage(volume, branchStart, branchEnd, branchLength, dx, dz, numVines, minLen, maxLen, leavesColor)
	for v = 1, numVines do
		-- Distribute strands along the branch
		local t = (v - 0.5) / numVines
		t = 0.3 + t * 0.7 -- Start hanging from 30% along the branch outwards
		local vineX = math.floor(branchStart.x + (branchEnd.x - branchStart.x) * t + (math.random() - 0.5) * 2)
		local vineZ = math.floor(branchStart.z + (branchEnd.z - branchStart.z) * t + (math.random() - 0.5) * 2)
		local vineY = math.floor(branchStart.y + (branchEnd.y - branchStart.y) * t)

		local vineLength = math.random(minLen, maxLen)
		local vineStart = g_ivec3.new(vineX, vineY, vineZ)

		-- Vines sway slightly sideways
		local swayX = math.random(-2, 2)
		local swayZ = math.random(-2, 2)
		local vineEnd = g_ivec3.new(vineX + swayX, vineY - vineLength, vineZ + swayZ)
		local vineCtrl = g_ivec3.new(
			vineX + math.floor(swayX * 0.3),
			vineY - math.floor(vineLength * 0.4),
			vineZ + math.floor(swayZ * 0.3)
		)
		drawBezier(volume, vineStart, vineEnd, vineCtrl, 1, 1, math.max(4, vineLength), leavesColor)
	end
end

-- Create a foliage canopy dome at the crown
local function createCanopy(volume, crownCenter, crownSpread, voxelColor)
	local canopyHeight = math.max(3, math.floor(crownSpread * 0.3))
	-- Main canopy - broad and somewhat flat
	g_shape.dome(volume, crownCenter, 'y', false, crownSpread, canopyHeight, crownSpread, voxelColor)
	-- Slight irregularity with offset smaller domes
	for _ = 1, math.random(2, 4) do
		local offX = math.random(-math.floor(crownSpread / 4), math.floor(crownSpread / 4))
		local offZ = math.random(-math.floor(crownSpread / 4), math.floor(crownSpread / 4))
		local offY = math.random(-1, 1)
		local subW = math.random(math.floor(crownSpread * 0.3), math.floor(crownSpread * 0.5))
		local subH = math.random(2, math.max(3, canopyHeight - 1))
		local subCenter = g_ivec3.new(crownCenter.x + offX, crownCenter.y + offY, crownCenter.z + offZ)
		g_shape.dome(volume, subCenter, 'y', false, subW, subH, subW, voxelColor)
	end
end

function main(node, region, color, trunkHeight, trunkStrength, crownSpread, mainBranches,
	branchLength, droopLength, hangingVines, vineMinLength, vineMaxLength,
	forkChance, trunkColor, leavesColor, seed)

	tree_utils.initSeed(seed)

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Create root flare at the base
	createRootFlare(volume, pos, trunkStrength, trunkColor)

	-- Create main trunk
	local trunkTop, topThickness = createTrunk(volume, pos, trunkHeight, trunkStrength, trunkColor)

	-- Optionally fork the trunk (characteristic of black willow)
	local forkTops = {}
	local roll = math.random(1, 100)
	if roll <= forkChance then
		local forkY = math.floor(trunkHeight * (0.3 + math.random() * 0.2))
		local forkPos = g_ivec3.new(
			pos.x,
			pos.y + forkY,
			pos.z
		)
		local forkThick = math.max(1, math.floor(trunkStrength * 0.5))
		local forkTop, forkTopThick = createFork(volume, forkPos, trunkHeight, forkThick, trunkColor)
		table.insert(forkTops, { pos = forkTop, thickness = forkTopThick })
	end

	-- Canopy at the top of the main trunk
	createCanopy(volume, trunkTop, crownSpread, leavesColor)

	-- Create main arching branches from the crown
	local angleStep = (2 * math.pi) / mainBranches
	local startAngle = math.random() * 2 * math.pi

	for i = 1, mainBranches do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.5

		-- Branches originate from slightly below the crown top
		local branchStartY = trunkTop.y - math.random(0, 3)
		local branchOrigin = g_ivec3.new(trunkTop.x, branchStartY, trunkTop.z)

		-- Vary branch length for natural look
		local thisLength = branchLength + math.random(-2, 3)
		local thisDroop = droopLength + math.random(-2, 3)
		thisDroop = math.max(2, thisDroop)
		thisLength = math.max(3, thisLength)

		createArchingBranch(volume, branchOrigin, angle, thisLength, thisDroop,
			topThickness, trunkColor, leavesColor, hangingVines, vineMinLength, vineMaxLength)
	end

	-- Add branches from fork tops too
	for _, fork in ipairs(forkTops) do
		local forkBranches = math.random(2, math.max(3, math.floor(mainBranches * 0.5)))
		local forkAngleStep = (2 * math.pi) / forkBranches
		local forkStartAngle = math.random() * 2 * math.pi
		-- Smaller canopy on fork
		local forkCrownSpread = math.max(4, math.floor(crownSpread * 0.6))
		createCanopy(volume, fork.pos, forkCrownSpread, leavesColor)

		for j = 1, forkBranches do
			local angle = forkStartAngle + (j - 1) * forkAngleStep + (math.random() - 0.5) * 0.5
			local branchOrigin = g_ivec3.new(fork.pos.x, fork.pos.y - math.random(0, 2), fork.pos.z)
			local thisLength = math.max(3, math.floor(branchLength * 0.7) + math.random(-1, 2))
			local thisDroop = math.max(2, math.floor(droopLength * 0.7) + math.random(-1, 2))
			createArchingBranch(volume, branchOrigin, angle, thisLength, thisDroop,
				fork.thickness, trunkColor, leavesColor,
				math.max(1, hangingVines - 1), vineMinLength, vineMaxLength)
		end
	end

	-- Additional hanging vines from the canopy edges for that cascading look
	local numExtraVines = math.random(math.floor(mainBranches * 1.5), mainBranches * 3)
	local extraAngle = math.random() * 2 * math.pi
	local extraAngleStep = (2 * math.pi) / numExtraVines
	for i = 1, numExtraVines do
		local angle = extraAngle + (i - 1) * extraAngleStep + (math.random() - 0.5) * 0.3
		local radius = math.random(math.floor(crownSpread * 0.2), math.floor(crownSpread * 0.45))
		local vineX = math.floor(trunkTop.x + math.cos(angle) * radius)
		local vineZ = math.floor(trunkTop.z + math.sin(angle) * radius)
		local vineY = trunkTop.y + math.random(-1, 2)
		local vineLength = math.random(vineMinLength, vineMaxLength)
		local vineStart = g_ivec3.new(vineX, vineY, vineZ)
		local swayX = math.random(-2, 2)
		local swayZ = math.random(-2, 2)
		local vineEnd = g_ivec3.new(vineX + swayX, vineY - vineLength, vineZ + swayZ)
		local vineCtrl = g_ivec3.new(
			vineX + math.floor(swayX * 0.4),
			vineY - math.floor(vineLength * 0.3),
			vineZ + math.floor(swayZ * 0.4)
		)
		drawBezier(volume, vineStart, vineEnd, vineCtrl, 1, 1, math.max(4, vineLength), leavesColor)
	end
end
