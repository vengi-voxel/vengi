--
-- American Beech tree generator
-- Creates an American Beech with its characteristic smooth silver-gray bark,
-- short stout trunk, wide spreading horizontal branches, and a dense, layered
-- canopy of oval leaves that retain through winter (marcescent).
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '14', min = '6', max = '30' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'trunkCurve', desc = 'Slight trunk lean', type = 'int', default = '1', min = '0', max = '4' },
		{ name = 'mainBranches', desc = 'Number of main spreading branches', type = 'int', default = '6', min = '3', max = '10' },
		{ name = 'branchLength', desc = 'Length of main branches', type = 'int', default = '10', min = '4', max = '18' },
		{ name = 'subBranches', desc = 'Sub-branches per main branch', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'canopySpread', desc = 'Overall canopy width', type = 'int', default = '12', min = '5', max = '22' },
		{ name = 'canopyDensity', desc = 'Foliage density (1=sparse, 5=very dense)', type = 'int', default = '4', min = '1', max = '5' },
		{ name = 'lowBranches', desc = 'Include low sweeping branches near ground', type = 'bool', default = 'true' },
		{ name = 'trunkColor', desc = 'Bark color (smooth silver-gray)', type = 'hexcolor', default = '#A9A9A9' },
		{ name = 'branchColor', desc = 'Branch color', type = 'hexcolor', default = '#8B8378' },
		{ name = 'leafColor', desc = 'Primary leaf color', type = 'hexcolor', default = '#3B7A2E' },
		{ name = 'leafColor2', desc = 'Secondary leaf color (lighter)', type = 'hexcolor', default = '#5CA04A' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates an American Beech with smooth gray bark, wide spreading branches and a dense layered canopy'
end

local drawBezier = tree_utils.drawBezier

local leafCluster = tree_utils.leafCluster

-- Create a main spreading branch with sub-branches and foliage
local function createBranch(volume, origin, angle, length, branchColor,
	leafColor, leafColor2, subBranches, canopyDensity, isLowBranch)

	local dx = math.cos(angle)
	local dz = math.sin(angle)

	-- Beech branches grow outward nearly horizontally, slight upward curve
	local rise = math.random(1, 3)
	if isLowBranch then
		rise = math.random(-1, 1) -- low branches can even dip slightly
	end

	local branchEnd = g_ivec3.new(
		math.floor(origin.x + dx * length),
		origin.y + rise,
		math.floor(origin.z + dz * length)
	)
	local ctrl = g_ivec3.new(
		math.floor(origin.x + dx * length * 0.4),
		origin.y + rise + math.random(1, 3),
		math.floor(origin.z + dz * length * 0.4)
	)

	local branchThick = math.max(1, math.floor(length * 0.15) + 1)
	local tip = drawBezier(volume, origin, branchEnd, ctrl, branchThick, 1,
		math.max(6, length), branchColor)

	-- Foliage at tip
	leafCluster(volume, tip, math.max(3, math.floor(length * 0.45)), leafColor, leafColor2)

	-- Sub-branches
	for s = 1, subBranches do
		local subT = 0.3 + (s - 1) * (0.5 / math.max(1, subBranches - 1))
		if subT > 0.9 then subT = 0.9 end
		local subU = 1.0 - subT

		local subOrigin = g_ivec3.new(
			math.floor(subU * subU * origin.x + 2 * subU * subT * ctrl.x + subT * subT * branchEnd.x),
			math.floor(subU * subU * origin.y + 2 * subU * subT * ctrl.y + subT * subT * branchEnd.y),
			math.floor(subU * subU * origin.z + 2 * subU * subT * ctrl.z + subT * subT * branchEnd.z)
		)

		local subAngle = angle + (math.random() - 0.5) * 1.8
		local subDx = math.cos(subAngle)
		local subDz = math.sin(subAngle)
		local subLen = math.max(2, math.floor(length * (0.3 + math.random() * 0.25)))
		local subRise = math.random(-1, 2)

		local subEnd = g_ivec3.new(
			math.floor(subOrigin.x + subDx * subLen),
			subOrigin.y + subRise,
			math.floor(subOrigin.z + subDz * subLen)
		)
		g_shape.line(volume, subOrigin, subEnd, branchColor, 1)

		-- Foliage at sub-branch
		leafCluster(volume, subEnd, math.max(2, math.floor(subLen * 0.5)), leafColor, leafColor2)

		-- Extra midpoint foliage for dense canopy
		if canopyDensity >= 3 then
			local midPos = g_ivec3.new(
				math.floor((subOrigin.x + subEnd.x) / 2),
				math.floor((subOrigin.y + subEnd.y) / 2) + 1,
				math.floor((subOrigin.z + subEnd.z) / 2)
			)
			leafCluster(volume, midPos, math.max(2, math.floor(subLen * 0.35)), leafColor, leafColor2)
		end

		-- Tertiary twigs for very dense canopy
		if canopyDensity >= 4 and math.random() > 0.4 then
			local tAngle = subAngle + (math.random() - 0.5) * 1.5
			local tLen = math.max(1, math.floor(subLen * 0.4))
			local tEnd = g_ivec3.new(
				math.floor(subEnd.x + math.cos(tAngle) * tLen),
				subEnd.y + math.random(0, 1),
				math.floor(subEnd.z + math.sin(tAngle) * tLen)
			)
			g_shape.line(volume, subEnd, tEnd, branchColor, 1)
			leafCluster(volume, tEnd, math.max(1, math.floor(tLen * 0.6)), leafColor, leafColor2)
		end
	end

	-- Fill foliage along the main branch for density
	if canopyDensity >= 2 then
		local numFill = math.min(canopyDensity, 4)
		for e = 1, numFill do
			local eT = 0.2 + e * (0.6 / numFill)
			local eU = 1.0 - eT
			local ePos = g_ivec3.new(
				math.floor(eU * eU * origin.x + 2 * eU * eT * ctrl.x + eT * eT * branchEnd.x),
				math.floor(eU * eU * origin.y + 2 * eU * eT * ctrl.y + eT * eT * branchEnd.y) + 1,
				math.floor(eU * eU * origin.z + 2 * eU * eT * ctrl.z + eT * eT * branchEnd.z)
			)
			leafCluster(volume, ePos, math.max(2, math.floor(length * 0.3)), leafColor, leafColor2)
		end
	end
end

function main(node, region, color, trunkHeight, trunkStrength, trunkCurve,
	mainBranches, branchLength, subBranches, canopySpread, canopyDensity, lowBranches,
	trunkColor, branchColor, leafColor, leafColor2, seed)

	tree_utils.initSeed(seed)

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Smooth trunk with slight lean — beech trunks are famously smooth
	local curveDir = math.random() * 2 * math.pi
	local curveX = math.floor(math.cos(curveDir) * trunkCurve)
	local curveZ = math.floor(math.sin(curveDir) * trunkCurve)
	local topPos = g_ivec3.new(pos.x + curveX, pos.y + trunkHeight, pos.z + curveZ)
	local ctrl = g_ivec3.new(
		pos.x + math.floor(curveX * 0.4),
		pos.y + math.floor(trunkHeight * 0.5),
		pos.z + math.floor(curveZ * 0.4)
	)
	drawBezier(volume, pos, topPos, ctrl, trunkStrength, math.max(1, trunkStrength - 1),
		trunkHeight, trunkColor)

	-- Prominent root flare — beech trees have broad, shallow root systems
	g_shape.dome(volume, pos, 'y', false,
		(trunkStrength + 3) * 2, math.max(2, trunkStrength), (trunkStrength + 3) * 2, trunkColor)

	-- Surface roots — beech roots often protrude above ground
	for _ = 1, math.random(3, 6) do
		local rAngle = math.random() * 2 * math.pi
		local rLen = math.random(2, trunkStrength + 3)
		local rootEnd = g_ivec3.new(
			math.floor(pos.x + math.cos(rAngle) * rLen),
			pos.y - math.random(0, 1),
			math.floor(pos.z + math.sin(rAngle) * rLen)
		)
		g_shape.line(volume, pos, rootEnd, trunkColor, math.max(1, trunkStrength - 1))
	end

	-- Main spreading branches — beech trees have a wide, layered crown
	local angleStep = (2 * math.pi) / mainBranches
	local startAngle = math.random() * 2 * math.pi

	for i = 1, mainBranches do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.4
		local bLen = branchLength + math.random(-2, 2)
		bLen = math.max(4, bLen)

		-- Vary origin height along the upper trunk
		local originFrac = 0.5 + math.random() * 0.4
		local oU = 1.0 - originFrac
		local branchOrigin = g_ivec3.new(
			math.floor(oU * oU * pos.x + 2 * oU * originFrac * ctrl.x + originFrac * originFrac * topPos.x),
			math.floor(oU * oU * pos.y + 2 * oU * originFrac * ctrl.y + originFrac * originFrac * topPos.y),
			math.floor(oU * oU * pos.z + 2 * oU * originFrac * ctrl.z + originFrac * originFrac * topPos.z)
		)

		createBranch(volume, branchOrigin, angle, bLen, branchColor,
			leafColor, leafColor2, subBranches, canopyDensity, false)
	end

	-- Low sweeping branches — beech trees often have branches close to the ground
	if lowBranches then
		local numLow = math.random(2, 3)
		for i = 1, numLow do
			local lowAngle = startAngle + math.pi / mainBranches + (i - 1) * (2 * math.pi / numLow)
			local lowFrac = 0.2 + math.random() * 0.15
			local lowU = 1.0 - lowFrac
			local lowOrigin = g_ivec3.new(
				math.floor(lowU * lowU * pos.x + 2 * lowU * lowFrac * ctrl.x + lowFrac * lowFrac * topPos.x),
				math.floor(lowU * lowU * pos.y + 2 * lowU * lowFrac * ctrl.y + lowFrac * lowFrac * topPos.y),
				math.floor(lowU * lowU * pos.z + 2 * lowU * lowFrac * ctrl.z + lowFrac * lowFrac * topPos.z)
			)
			local lowLen = math.max(3, branchLength - math.random(2, 4))
			createBranch(volume, lowOrigin, lowAngle, lowLen, branchColor,
				leafColor, leafColor2, math.max(1, subBranches - 1), canopyDensity, true)
		end
	end

	-- Dense central canopy dome — beech canopies are very full
	local canopyCenterY = topPos.y + math.floor(canopySpread * 0.1)
	local canopyCenter = g_ivec3.new(topPos.x, canopyCenterY, topPos.z)
	local cW = canopySpread
	local cH = math.max(3, math.floor(canopySpread * 0.45))
	g_shape.dome(volume, canopyCenter, 'y', false, cW * 2, cH, cW * 2, leafColor)
	-- Top layer in secondary color
	local topCap = g_ivec3.new(canopyCenter.x, canopyCenter.y + 1, canopyCenter.z)
	g_shape.dome(volume, topCap, 'y', false, math.floor(cW * 1.2), math.max(1, cH - 1),
		math.floor(cW * 1.2), leafColor2)
	-- Under-canopy fill
	g_shape.dome(volume, canopyCenter, 'y', true,
		math.floor(cW * 0.7), math.max(2, cH - 2), math.floor(cW * 0.7), leafColor)
end
