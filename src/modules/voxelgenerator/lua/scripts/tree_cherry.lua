--
-- Blossoming cherry tree (Sakura) generator
-- Creates a cherry tree with a graceful spreading canopy, gnarled trunk,
-- arching branches, dense clusters of pink/white blossoms, and scattered petals.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '16', min = '6', max = '35' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'trunkCurve', desc = 'Trunk lean amount', type = 'int', default = '2', min = '0', max = '6' },
		{ name = 'canopySpread', desc = 'Horizontal spread of the canopy', type = 'int', default = '12', min = '5', max = '22' },
		{ name = 'canopyHeight', desc = 'Vertical thickness of canopy', type = 'int', default = '8', min = '3', max = '14' },
		{ name = 'mainBranches', desc = 'Number of main branches', type = 'int', default = '5', min = '3', max = '8' },
		{ name = 'subBranches', desc = 'Sub-branches per main branch', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'blossomDensity', desc = 'Density of blossom clusters', type = 'int', default = '3', min = '1', max = '5' },
		{ name = 'blossomSize', desc = 'Size of individual blossom clusters', type = 'int', default = '4', min = '2', max = '8' },
		{ name = 'fallenPetals', desc = 'Scatter fallen petals on the ground', type = 'bool', default = 'true' },
		{ name = 'trunkColor', desc = 'Color of the trunk/bark', type = 'hexcolor', default = '#5C3A21' },
		{ name = 'branchColor', desc = 'Color of smaller branches', type = 'hexcolor', default = '#6B4226' },
		{ name = 'blossomColor1', desc = 'Primary blossom color (pink)', type = 'hexcolor', default = '#FFB7C5' },
		{ name = 'blossomColor2', desc = 'Secondary blossom color (white/lighter)', type = 'hexcolor', default = '#FFF0F5' },
		{ name = 'blossomColor3', desc = 'Accent blossom color (deeper pink)', type = 'hexcolor', default = '#FF69B4' },
		{ name = 'leafColor', desc = 'Young leaf color (sparse among blossoms)', type = 'hexcolor', default = '#7CCD7C' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a blossoming cherry tree (sakura) with arching branches and dense pink/white blossom clusters'
end

local drawBezier = tree_utils.drawBezier

-- Place a blossom cluster — a dome of pink/white with occasional leaf accents
local function placeBlossom(volume, center, size, blossomColor1, blossomColor2, blossomColor3, leafColor)
	local w = size + math.random(-1, 1)
	local h = math.max(1, math.floor(size * 0.7) + math.random(-1, 0))
	w = math.max(2, w)

	-- Pick a blossom color weighted toward primary pink
	local r = math.random()
	local col
	if r < 0.50 then
		col = blossomColor1
	elseif r < 0.78 then
		col = blossomColor2
	else
		col = blossomColor3
	end

	g_shape.dome(volume, center, 'y', false, w * 2, h, w * 2, col)

	-- Optional small underside fill for fullness
	if size >= 3 then
		local fillCol = blossomColor1
		if math.random() > 0.6 then
			fillCol = blossomColor3
		end
		g_shape.dome(volume, center, 'y', true, math.floor(w * 0.8), math.max(1, h - 1), math.floor(w * 0.8), fillCol)
	end

	-- Sparse young leaves poking through
	if math.random() > 0.7 then
		local leafOff = g_ivec3.new(
			center.x + math.random(-1, 1),
			center.y + math.random(0, 1),
			center.z + math.random(-1, 1)
		)
		g_shape.dome(volume, leafOff, 'y', false, math.max(2, w - 1), math.max(1, h - 1), math.max(2, w - 1), leafColor)
	end
end

-- Create a main branch that arches upward then curves outward/downward — cherry style
local function createMainBranch(volume, origin, angle, length, archHeight, branchColor, trunkColor,
	blossomColor1, blossomColor2, blossomColor3, leafColor, subBranches, blossomDensity, blossomSize)

	local dx = math.cos(angle)
	local dz = math.sin(angle)

	-- Cherry branches arch upward then spread outward with graceful curves
	local rise = archHeight + math.random(-1, 2)
	local spreadFactor = 0.85 + math.random() * 0.3

	local branchEnd = g_ivec3.new(
		math.floor(origin.x + dx * length * spreadFactor),
		origin.y + rise - math.random(0, 2),
		math.floor(origin.z + dz * length * spreadFactor)
	)
	local ctrl = g_ivec3.new(
		math.floor(origin.x + dx * length * 0.35),
		origin.y + rise + math.random(1, 3),
		math.floor(origin.z + dz * length * 0.35)
	)

	local branchThick = math.max(1, math.floor(length * 0.15))
	local tip = drawBezier(volume, origin, branchEnd, ctrl, branchThick, 1, math.max(8, length), branchColor)

	-- Blossom cluster at the tip of main branch
	placeBlossom(volume, tip, blossomSize, blossomColor1, blossomColor2, blossomColor3, leafColor)

	-- Sub-branches forking off the main branch
	for s = 1, subBranches do
		local subT = 0.3 + (s - 1) * (0.5 / math.max(1, subBranches - 1))
		if subT > 0.95 then subT = 0.95 end

		-- Position along the main branch curve
		local subOrigin = tree_utils.bezierPointAt(origin, branchEnd, ctrl, subT)

		-- Sub-branch direction: diverge from main branch angle
		local subAngle = angle + (math.random() - 0.5) * 2.0
		local subDx = math.cos(subAngle)
		local subDz = math.sin(subAngle)
		local subLen = math.max(3, math.floor(length * (0.3 + math.random() * 0.25)))
		local subRise = math.random(-1, 2)

		local subEnd = g_ivec3.new(
			math.floor(subOrigin.x + subDx * subLen),
			subOrigin.y + subRise,
			math.floor(subOrigin.z + subDz * subLen)
		)
		local subCtrl = g_ivec3.new(
			math.floor(subOrigin.x + subDx * subLen * 0.4),
			subOrigin.y + subRise + math.random(1, 2),
			math.floor(subOrigin.z + subDz * subLen * 0.4)
		)

		local subTip = drawBezier(volume, subOrigin, subEnd, subCtrl, 1, 1, math.max(5, subLen), branchColor)

		-- Blossom at sub-branch tip
		placeBlossom(volume, subTip, math.max(2, blossomSize - 1), blossomColor1, blossomColor2, blossomColor3, leafColor)

		-- Extra blossom at midpoint for denser appearance
		if blossomDensity >= 3 and subLen >= 4 then
			local midPos = g_ivec3.new(
				math.floor((subOrigin.x + subEnd.x) / 2),
				math.floor((subOrigin.y + subEnd.y) / 2) + 1,
				math.floor((subOrigin.z + subEnd.z) / 2)
			)
			placeBlossom(volume, midPos, math.max(2, blossomSize - 2), blossomColor1, blossomColor2, blossomColor3, leafColor)
		end

		-- Drooping twig tips at the end of sub-branches (cherry characteristic)
		if math.random() > 0.4 then
			local droopEnd = g_ivec3.new(
				subTip.x + math.random(-1, 1),
				subTip.y - math.random(1, 3),
				subTip.z + math.random(-1, 1)
			)
			g_shape.line(volume, subTip, droopEnd, branchColor, 1)
			-- Tiny blossom at droop tip
			if blossomDensity >= 2 then
				local tinySize = math.max(1, blossomSize - 2)
				placeBlossom(volume, droopEnd, tinySize, blossomColor1, blossomColor2, blossomColor3, leafColor)
			end
		end
	end

	-- Extra blossom clusters along the main branch for very dense canopies
	if blossomDensity >= 4 then
		for extra = 1, blossomDensity - 2 do
			local eT = 0.2 + extra * (0.6 / (blossomDensity - 2))
			local ePos = tree_utils.bezierPointAt(origin, branchEnd, ctrl, eT)
			ePos.y = ePos.y + 1
			placeBlossom(volume, ePos, math.max(2, blossomSize - 1), blossomColor1, blossomColor2, blossomColor3, leafColor)
		end
	end
end

function main(node, region, color, trunkHeight, trunkStrength, trunkCurve, canopySpread,
	canopyHeight, mainBranches, subBranches, blossomDensity, blossomSize, fallenPetals,
	trunkColor, branchColor, blossomColor1, blossomColor2, blossomColor3, leafColor, seed)

	tree_utils.initSeed(seed)

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Gnarled trunk with a characteristic lean
	local topPos, _, ctrl = tree_utils.createCurvedTrunk(volume, pos, trunkHeight, trunkStrength,
		trunkCurve, math.max(1, trunkStrength - 1), trunkColor, 0.6)

	-- Root flare at the base
	tree_utils.createBaseFlare(volume, pos, trunkStrength + 2, math.max(1, trunkStrength - 1), trunkColor)

	-- Exposed root bumps
	tree_utils.createLineRoots(volume, pos, math.random(2, 4), trunkStrength + 2,
		math.max(1, trunkStrength - 1), trunkColor, true)

	-- Fork point — cherry trees often split into main scaffold branches
	local forkY = pos.y + math.floor(trunkHeight * 0.55)
	local forkHt = (forkY - pos.y) / trunkHeight
	local forkPos = tree_utils.bezierPointAt(pos, topPos, ctrl, forkHt)

	-- Main scaffold branches radiating from the fork zone and trunk top
	local angleStep = (2 * math.pi) / mainBranches
	local startAngle = math.random() * 2 * math.pi

	for i = 1, mainBranches do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.4

		-- Alternate origin between fork point and trunk top for varied heights
		local branchOrigin
		if i % 2 == 0 then
			branchOrigin = forkPos
		else
			-- Slightly below the very top
			local originT = 0.7 + math.random() * 0.2
			branchOrigin = tree_utils.bezierPointAt(pos, topPos, ctrl, originT)
		end

		local branchLen = canopySpread + math.random(-2, 2)
		local archHeight = math.floor(canopyHeight * 0.5) + math.random(-1, 2)

		createMainBranch(volume, branchOrigin, angle, branchLen, archHeight, branchColor, trunkColor,
			blossomColor1, blossomColor2, blossomColor3, leafColor, subBranches, blossomDensity, blossomSize)
	end

	-- Central canopy fill — a large blossom mass at the top center
	local canopyCenterY = topPos.y + math.floor(canopyHeight * 0.3)
	local canopyCenter = g_ivec3.new(topPos.x, canopyCenterY, topPos.z)
	local centralW = math.floor(canopySpread * 0.6)
	local centralH = math.max(2, math.floor(canopyHeight * 0.4))
	g_shape.dome(volume, canopyCenter, 'y', false, centralW * 2, centralH, centralW * 2, blossomColor1)
	-- Secondary color layer slightly above
	local topCap = g_ivec3.new(canopyCenter.x, canopyCenter.y + 1, canopyCenter.z)
	g_shape.dome(volume, topCap, 'y', false, math.floor(centralW * 1.4), math.max(1, centralH - 1), math.floor(centralW * 1.4), blossomColor2)

	-- Fallen petals scattered on the ground
	if fallenPetals then
		local groundY = pos.y
		local scatterRadius = canopySpread + 3
		local petalCount = scatterRadius * 2 + math.random(5, 15)
		for _ = 1, petalCount do
			local px = pos.x + math.random(-scatterRadius, scatterRadius)
			local pz = pos.z + math.random(-scatterRadius, scatterRadius)
			-- Only place if roughly under the canopy
			local dist = math.sqrt((px - pos.x) ^ 2 + (pz - pos.z) ^ 2)
			if dist <= scatterRadius then
				local petalPos = g_ivec3.new(px, groundY, pz)
				local petalCol = blossomColor1
				local pr = math.random()
				if pr > 0.7 then
					petalCol = blossomColor2
				elseif pr > 0.5 then
					petalCol = blossomColor3
				end
				g_shape.line(volume, petalPos, petalPos, petalCol, 1)
			end
		end
	end
end
