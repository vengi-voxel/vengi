--
-- Hawthorn tree generator
-- Creates a hawthorn with a dense, rounded, often irregular crown, short
-- gnarled trunk that may fork low, thorny branches, clusters of white/pink
-- blossoms, and red berry clusters (haws).
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '10', min = '4', max = '25' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '2', min = '1', max = '5' },
		{ name = 'trunkCurve', desc = 'Trunk lean / gnarled twist', type = 'int', default = '2', min = '0', max = '6' },
		{ name = 'forkHeight', desc = 'Height fraction where trunk forks (%)', type = 'int', default = '35', min = '20', max = '60' },
		{ name = 'mainBranches', desc = 'Number of main scaffold branches', type = 'int', default = '5', min = '3', max = '8' },
		{ name = 'branchLength', desc = 'Length of main branches', type = 'int', default = '8', min = '3', max = '16' },
		{ name = 'subBranches', desc = 'Sub-branches per main branch', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'canopySize', desc = 'Overall canopy dome size', type = 'int', default = '10', min = '4', max = '20' },
		{ name = 'thorns', desc = 'Show thorn spurs on branches', type = 'bool', default = 'true' },
		{ name = 'blossoms', desc = 'Show blossom clusters', type = 'bool', default = 'true' },
		{ name = 'berries', desc = 'Show red berry clusters (haws)', type = 'bool', default = 'true' },
		{ name = 'trunkColor', desc = 'Bark color', type = 'hexcolor', default = '#5C4033' },
		{ name = 'branchColor', desc = 'Smaller branch color', type = 'hexcolor', default = '#6B4226' },
		{ name = 'thornColor', desc = 'Thorn spur color', type = 'hexcolor', default = '#8B7355' },
		{ name = 'leafColor', desc = 'Primary leaf color', type = 'hexcolor', default = '#3A6B35' },
		{ name = 'leafColor2', desc = 'Secondary leaf color (lighter)', type = 'hexcolor', default = '#5B8C3E' },
		{ name = 'blossomColor', desc = 'Blossom color (white / pink)', type = 'hexcolor', default = '#FFF5EE' },
		{ name = 'berryColor', desc = 'Berry (haw) color', type = 'hexcolor', default = '#CC2233' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a hawthorn tree with a gnarled trunk, dense irregular crown, thorns, blossoms and red berries'
end

-- Quadratic bezier, returns final position
local function drawBezier(volume, startPos, endPos, control, startThick, endThick, steps, col)
	local last = startPos
	for i = 1, steps do
		local t = i / steps
		local u = 1.0 - t
		local p = g_ivec3.new(
			math.floor(u * u * startPos.x + 2 * u * t * control.x + t * t * endPos.x),
			math.floor(u * u * startPos.y + 2 * u * t * control.y + t * t * endPos.y),
			math.floor(u * u * startPos.z + 2 * u * t * control.z + t * t * endPos.z)
		)
		local th = math.max(1, math.ceil(startThick + (endThick - startThick) * t))
		g_shape.line(volume, last, p, col, th)
		last = p
	end
	return last
end

-- Place a leaf cluster dome
local function leafCluster(volume, center, size, leafColor, leafColor2)
	local w = math.max(2, size + math.random(-1, 1))
	local h = math.max(1, math.floor(w * 0.65) + math.random(-1, 0))
	local col = leafColor
	if math.random() > 0.55 then col = leafColor2 end
	g_shape.dome(volume, center, 'y', false, w * 2, h, w * 2, col)
	-- Under-fill for volume
	if size >= 3 then
		g_shape.dome(volume, center, 'y', true,
			math.floor(w * 0.9), math.max(1, h - 1), math.floor(w * 0.9), leafColor)
	end
end

-- Place a small blossom or berry cluster
local function smallCluster(volume, center, size, col)
	local w = math.max(1, size)
	local h = math.max(1, math.floor(w * 0.5))
	g_shape.dome(volume, center, 'y', false, w * 2, h, w * 2, col)
end

-- Place thorns — short spikes protruding from a branch point
local function placeThorns(volume, pos, thornColor)
	local numThorns = math.random(1, 3)
	for _ = 1, numThorns do
		local tAngle = math.random() * 2 * math.pi
		local dx = math.cos(tAngle)
		local dz = math.sin(tAngle)
		local len = math.random(1, 2)
		local thornTip = g_ivec3.new(
			math.floor(pos.x + dx * len),
			pos.y + math.random(-1, 1),
			math.floor(pos.z + dz * len)
		)
		g_shape.line(volume, pos, thornTip, thornColor, 1)
	end
end

-- Create a main branch with sub-branches, foliage, blossoms, berries
local function createBranch(volume, origin, angle, length, branchColor, thornColor,
	leafColor, leafColor2, blossomColor, berryColor,
	subBranches, showThorns, showBlossoms, showBerries)

	local dx = math.cos(angle)
	local dz = math.sin(angle)

	-- Hawthorn branches grow outward and slightly upward, somewhat crooked
	local rise = math.random(1, math.max(2, math.floor(length * 0.3)))
	local lateralJitter = (math.random() - 0.5) * 2

	local branchEnd = g_ivec3.new(
		math.floor(origin.x + dx * length),
		origin.y + rise - math.random(0, 2),
		math.floor(origin.z + dz * length)
	)
	local ctrl = g_ivec3.new(
		math.floor(origin.x + dx * length * 0.4 + lateralJitter),
		origin.y + rise + math.random(0, 2),
		math.floor(origin.z + dz * length * 0.4 + lateralJitter)
	)

	local branchThick = math.max(1, math.floor(length * 0.12) + 1)
	local tip = drawBezier(volume, origin, branchEnd, ctrl, branchThick, 1,
		math.max(6, length), branchColor)

	-- Leaf cluster at tip
	leafCluster(volume, tip, math.max(2, math.floor(length * 0.45)), leafColor, leafColor2)

	-- Thorns along the main branch
	if showThorns and length >= 4 then
		for t = 1, math.random(1, 3) do
			local frac = 0.3 + (t - 1) * 0.25
			local u = 1.0 - frac
			local thornPos = g_ivec3.new(
				math.floor(u * u * origin.x + 2 * u * frac * ctrl.x + frac * frac * branchEnd.x),
				math.floor(u * u * origin.y + 2 * u * frac * ctrl.y + frac * frac * branchEnd.y),
				math.floor(u * u * origin.z + 2 * u * frac * ctrl.z + frac * frac * branchEnd.z)
			)
			placeThorns(volume, thornPos, thornColor)
		end
	end

	-- Sub-branches
	for s = 1, subBranches do
		local subT = 0.25 + (s - 1) * (0.55 / math.max(1, subBranches - 1))
		if subT > 0.95 then subT = 0.95 end
		local subU = 1.0 - subT

		local subOrigin = g_ivec3.new(
			math.floor(subU * subU * origin.x + 2 * subU * subT * ctrl.x + subT * subT * branchEnd.x),
			math.floor(subU * subU * origin.y + 2 * subU * subT * ctrl.y + subT * subT * branchEnd.y),
			math.floor(subU * subU * origin.z + 2 * subU * subT * ctrl.z + subT * subT * branchEnd.z)
		)

		local subAngle = angle + (math.random() - 0.5) * 2.2
		local subDx = math.cos(subAngle)
		local subDz = math.sin(subAngle)
		local subLen = math.max(2, math.floor(length * (0.3 + math.random() * 0.2)))
		local subRise = math.random(-1, 2)

		local subEnd = g_ivec3.new(
			math.floor(subOrigin.x + subDx * subLen),
			subOrigin.y + subRise,
			math.floor(subOrigin.z + subDz * subLen)
		)
		g_shape.line(volume, subOrigin, subEnd, branchColor, 1)

		-- Leaf cluster at sub-branch tip
		leafCluster(volume, subEnd, math.max(2, math.floor(subLen * 0.5)), leafColor, leafColor2)

		-- Blossom at sub-branch tip
		if showBlossoms and math.random() > 0.35 then
			local blossomPos = g_ivec3.new(subEnd.x, subEnd.y + 1, subEnd.z)
			smallCluster(volume, blossomPos, math.max(1, math.floor(subLen * 0.3)), blossomColor)
		end

		-- Berry cluster hanging below sub-branch
		if showBerries and math.random() > 0.5 then
			local berryPos = g_ivec3.new(
				subEnd.x + math.random(-1, 1),
				subEnd.y - math.random(0, 1),
				subEnd.z + math.random(-1, 1)
			)
			smallCluster(volume, berryPos, math.max(1, math.floor(subLen * 0.25)), berryColor)
		end

		-- Thorns on sub-branch
		if showThorns and math.random() > 0.4 then
			placeThorns(volume, subEnd, thornColor)
		end
	end

	-- Blossom at main branch tip
	if showBlossoms and math.random() > 0.3 then
		local blossomPos = g_ivec3.new(tip.x, tip.y + 1, tip.z)
		smallCluster(volume, blossomPos, math.max(1, math.floor(length * 0.25)), blossomColor)
	end

	-- Berry cluster near main branch tip
	if showBerries and math.random() > 0.4 then
		local berryPos = g_ivec3.new(
			tip.x + math.random(-1, 1),
			tip.y - 1,
			tip.z + math.random(-1, 1)
		)
		smallCluster(volume, berryPos, math.max(1, math.floor(length * 0.2)), berryColor)
	end
end

function main(node, region, color, trunkHeight, trunkStrength, trunkCurve, forkHeight,
	mainBranches, branchLength, subBranches, canopySize, thorns, blossoms, berries,
	trunkColor, branchColor, thornColor, leafColor, leafColor2, blossomColor, berryColor, seed)

	if seed == 0 then
		math.randomseed(os.time())
	else
		math.randomseed(seed)
	end

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Gnarled, leaning trunk
	local curveDir = math.random() * 2 * math.pi
	local curveX = math.floor(math.cos(curveDir) * trunkCurve)
	local curveZ = math.floor(math.sin(curveDir) * trunkCurve)
	local topPos = g_ivec3.new(pos.x + curveX, pos.y + trunkHeight, pos.z + curveZ)
	local ctrl = g_ivec3.new(
		pos.x + math.floor(curveX * 0.5),
		pos.y + math.floor(trunkHeight * 0.5),
		pos.z + math.floor(curveZ * 0.5)
	)
	drawBezier(volume, pos, topPos, ctrl, trunkStrength, math.max(1, trunkStrength - 1),
		trunkHeight, trunkColor)

	-- Root flare
	g_shape.dome(volume, pos, 'y', false,
		(trunkStrength + 2) * 2, math.max(1, trunkStrength), (trunkStrength + 2) * 2, trunkColor)

	-- Exposed surface roots
	for _ = 1, math.random(2, 4) do
		local rAngle = math.random() * 2 * math.pi
		local rLen = math.random(2, trunkStrength + 2)
		local rootEnd = g_ivec3.new(
			math.floor(pos.x + math.cos(rAngle) * rLen),
			pos.y - math.random(0, 1),
			math.floor(pos.z + math.sin(rAngle) * rLen)
		)
		g_shape.line(volume, pos, rootEnd, trunkColor, math.max(1, trunkStrength - 1))
	end

	-- Fork point — hawthorns often fork low
	local forkY = pos.y + math.floor(trunkHeight * forkHeight / 100)
	local forkHt = (forkY - pos.y) / trunkHeight
	local forkU = 1.0 - forkHt
	local forkX = math.floor(forkU * forkU * pos.x + 2 * forkU * forkHt * ctrl.x + forkHt * forkHt * topPos.x)
	local forkZ = math.floor(forkU * forkU * pos.z + 2 * forkU * forkHt * ctrl.z + forkHt * forkHt * topPos.z)
	local forkPos = g_ivec3.new(forkX, forkY, forkZ)

	-- Main scaffold branches radiating from the fork zone and trunk top
	local angleStep = (2 * math.pi) / mainBranches
	local startAngle = math.random() * 2 * math.pi

	for i = 1, mainBranches do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.5
		local bLen = branchLength + math.random(-2, 2)
		bLen = math.max(3, bLen)

		-- Alternate origins between fork and trunk top for varied heights
		local branchOrigin
		if i % 2 == 0 then
			branchOrigin = forkPos
		else
			local oT = 0.65 + math.random() * 0.25
			local oU = 1.0 - oT
			branchOrigin = g_ivec3.new(
				math.floor(oU * oU * pos.x + 2 * oU * oT * ctrl.x + oT * oT * topPos.x),
				math.floor(oU * oU * pos.y + 2 * oU * oT * ctrl.y + oT * oT * topPos.y),
				math.floor(oU * oU * pos.z + 2 * oU * oT * ctrl.z + oT * oT * topPos.z)
			)
		end

		createBranch(volume, branchOrigin, angle, bLen, branchColor, thornColor,
			leafColor, leafColor2, blossomColor, berryColor,
			subBranches, thorns, blossoms, berries)
	end

	-- Dense central canopy dome
	local canopyCenterY = topPos.y + math.floor(canopySize * 0.15)
	local canopyCenter = g_ivec3.new(topPos.x, canopyCenterY, topPos.z)
	local cW = canopySize
	local cH = math.max(2, math.floor(canopySize * 0.55))
	g_shape.dome(volume, canopyCenter, 'y', false, cW * 2, cH, cW * 2, leafColor)
	-- Secondary color layer
	local topCap = g_ivec3.new(canopyCenter.x, canopyCenter.y + 1, canopyCenter.z)
	g_shape.dome(volume, topCap, 'y', false, math.floor(cW * 1.2), math.max(1, cH - 1),
		math.floor(cW * 1.2), leafColor2)
	-- Under-fill
	g_shape.dome(volume, canopyCenter, 'y', true, math.floor(cW * 0.8),
		math.max(1, cH - 2), math.floor(cW * 0.8), leafColor)

	-- Scatter blossoms across the canopy surface
	if blossoms then
		local blossomCount = math.random(4, 8 + canopySize)
		for _ = 1, blossomCount do
			local bX = canopyCenter.x + math.random(-cW, cW)
			local bZ = canopyCenter.z + math.random(-cW, cW)
			local bY = canopyCenter.y + math.random(0, cH)
			local dist = math.sqrt((bX - canopyCenter.x) ^ 2 + (bZ - canopyCenter.z) ^ 2)
			if dist <= cW then
				smallCluster(volume, g_ivec3.new(bX, bY, bZ), math.random(1, 2), blossomColor)
			end
		end
	end

	-- Scatter berry clusters around/below the canopy
	if berries then
		local berryCount = math.random(3, 6 + math.floor(canopySize * 0.5))
		for _ = 1, berryCount do
			local bX = canopyCenter.x + math.random(-cW, cW)
			local bZ = canopyCenter.z + math.random(-cW, cW)
			local bY = canopyCenter.y + math.random(-cH, math.floor(cH * 0.3))
			local dist = math.sqrt((bX - canopyCenter.x) ^ 2 + (bZ - canopyCenter.z) ^ 2)
			if dist <= cW + 1 then
				smallCluster(volume, g_ivec3.new(bX, bY, bZ), math.random(1, 2), berryColor)
			end
		end
	end
end
