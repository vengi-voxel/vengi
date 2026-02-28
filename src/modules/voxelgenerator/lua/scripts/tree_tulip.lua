--
-- Tulip tree (Liriodendron tulipifera) generator
-- Creates a tulip tree with a tall straight columnar trunk, distinctive
-- tulip-shaped flowers, broad oval crown, and unique lobed leaves.
-- One of the tallest eastern hardwoods with a very straight trunk.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '24', min = '10', max = '45' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'crownStart', desc = 'Height where crown begins (% of trunk)', type = 'int', default = '55', min = '30', max = '75' },
		{ name = 'mainBranches', desc = 'Number of main branches', type = 'int', default = '6', min = '3', max = '10' },
		{ name = 'branchLength', desc = 'Length of main branches', type = 'int', default = '10', min = '4', max = '18' },
		{ name = 'subBranches', desc = 'Sub-branches per main branch', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'canopyWidth', desc = 'Overall canopy width', type = 'int', default = '12', min = '5', max = '22' },
		{ name = 'canopyDensity', desc = 'Foliage density (1=sparse, 5=dense)', type = 'int', default = '3', min = '1', max = '5' },
		{ name = 'flowers', desc = 'Show tulip-shaped flowers', type = 'bool', default = 'true' },
		{ name = 'trunkColor', desc = 'Bark color (gray-brown furrowed)', type = 'hexcolor', default = '#6B6158' },
		{ name = 'branchColor', desc = 'Branch color', type = 'hexcolor', default = '#7D7068' },
		{ name = 'leafColor', desc = 'Primary leaf color', type = 'hexcolor', default = '#4A8C3F' },
		{ name = 'leafColor2', desc = 'Secondary leaf color', type = 'hexcolor', default = '#6BAF5B' },
		{ name = 'flowerColor1', desc = 'Flower petal color (yellow-green)', type = 'hexcolor', default = '#C5E17A' },
		{ name = 'flowerColor2', desc = 'Flower center / orange band', type = 'hexcolor', default = '#FF8C00' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a tulip tree with a tall straight trunk, oval crown and tulip-shaped flowers'
end

local drawBezier = tree_utils.drawBezier

local leafCluster = tree_utils.leafCluster

-- Tulip-shaped flower: small cone (cup) with an orange ring at the base
local function placeFlower(volume, pos, flowerColor1, flowerColor2)
	-- Petal cup — a small upward cone in yellow-green
	g_shape.cone(volume, g_ivec3.new(pos.x, pos.y - 1, pos.z), 'y', false, 3, 3, 3, flowerColor1)
	-- Orange band at the base of the petals
	g_shape.line(volume, g_ivec3.new(pos.x - 1, pos.y, pos.z), g_ivec3.new(pos.x + 1, pos.y, pos.z), flowerColor2, 1)
	g_shape.line(volume, g_ivec3.new(pos.x, pos.y, pos.z - 1), g_ivec3.new(pos.x, pos.y, pos.z + 1), flowerColor2, 1)
	-- Central stamen dot
	g_shape.line(volume, g_ivec3.new(pos.x, pos.y + 1, pos.z), g_ivec3.new(pos.x, pos.y + 1, pos.z), flowerColor2, 1)
end

-- Create a main branch with sub-branches, foliage, and optional flowers
local function createBranch(volume, origin, angle, length, branchColor,
	leafColor, leafColor2, flowerColor1, flowerColor2, subBranches, canopyDensity, showFlowers)

	local dx = math.cos(angle)
	local dz = math.sin(angle)

	-- Tulip tree branches grow upward at moderate angles
	local rise = math.random(2, math.max(3, math.floor(length * 0.4)))

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

	local branchThick = math.max(1, math.floor(length * 0.13) + 1)
	local tip = drawBezier(volume, origin, branchEnd, ctrl, branchThick, 1,
		math.max(6, length), branchColor)

	-- Foliage at tip
	leafCluster(volume, tip, math.max(3, math.floor(length * 0.45)), leafColor, leafColor2)

	-- Flower at tip
	if showFlowers and math.random() > 0.5 then
		local flowerPos = g_ivec3.new(tip.x, tip.y + 2, tip.z)
		placeFlower(volume, flowerPos, flowerColor1, flowerColor2)
	end

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
		local subLen = math.max(2, math.floor(length * (0.3 + math.random() * 0.2)))
		local subRise = math.random(1, 3)

		local subEnd = g_ivec3.new(
			math.floor(subOrigin.x + subDx * subLen),
			subOrigin.y + subRise,
			math.floor(subOrigin.z + subDz * subLen)
		)
		g_shape.line(volume, subOrigin, subEnd, branchColor, 1)

		-- Foliage at sub-branch
		leafCluster(volume, subEnd, math.max(2, math.floor(subLen * 0.5)), leafColor, leafColor2)

		-- Flower at sub-branch tip
		if showFlowers and math.random() > 0.65 then
			local fPos = g_ivec3.new(subEnd.x, subEnd.y + 2, subEnd.z)
			placeFlower(volume, fPos, flowerColor1, flowerColor2)
		end

		-- Extra midpoint foliage
		if canopyDensity >= 3 then
			local midPos = g_ivec3.new(
				math.floor((subOrigin.x + subEnd.x) / 2),
				math.floor((subOrigin.y + subEnd.y) / 2) + 1,
				math.floor((subOrigin.z + subEnd.z) / 2)
			)
			leafCluster(volume, midPos, math.max(2, math.floor(subLen * 0.35)), leafColor, leafColor2)
		end
	end

	-- Extra foliage along main branch for density
	if canopyDensity >= 2 then
		local numFill = math.min(canopyDensity, 3)
		for e = 1, numFill do
			local eT = 0.25 + e * (0.5 / numFill)
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

function main(node, region, color, trunkHeight, trunkStrength, crownStart,
	mainBranches, branchLength, subBranches, canopyWidth, canopyDensity, flowers,
	trunkColor, branchColor, leafColor, leafColor2, flowerColor1, flowerColor2, seed)

	tree_utils.initSeed(seed)

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Tall, very straight trunk — tulip trees are among the straightest hardwoods
	tree_utils.createTrunk(volume, pos, trunkHeight, trunkStrength, trunkColor)

	-- Root flare
	g_shape.dome(volume, pos, 'y', false,
		(trunkStrength + 2) * 2, math.max(1, trunkStrength), (trunkStrength + 2) * 2, trunkColor)

	-- Buttress roots
	for _ = 1, math.random(3, 5) do
		local rAngle = math.random() * 2 * math.pi
		local rLen = math.random(2, trunkStrength + 2)
		local rootEnd = g_ivec3.new(
			math.floor(pos.x + math.cos(rAngle) * rLen),
			pos.y,
			math.floor(pos.z + math.sin(rAngle) * rLen)
		)
		g_shape.line(volume, pos, rootEnd, trunkColor, math.max(1, trunkStrength - 1))
	end

	local topPos = g_ivec3.new(pos.x, pos.y + trunkHeight, pos.z)

	-- Crown starts at crownStart% of the trunk
	local crownBaseY = pos.y + math.floor(trunkHeight * crownStart / 100)

	-- Main branches radiating from the crown zone
	local angleStep = (2 * math.pi) / mainBranches
	local startAngle = math.random() * 2 * math.pi

	for i = 1, mainBranches do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.4
		local bLen = branchLength + math.random(-2, 2)
		bLen = math.max(4, bLen)

		-- Vary origin height in the crown zone
		local originY = crownBaseY + math.random(0, trunkHeight - math.floor(trunkHeight * crownStart / 100))
		originY = math.min(originY, topPos.y - 1)
		local branchOrigin = g_ivec3.new(pos.x, originY, pos.z)

		createBranch(volume, branchOrigin, angle, bLen, branchColor,
			leafColor, leafColor2, flowerColor1, flowerColor2,
			subBranches, canopyDensity, flowers)
	end

	-- Oval canopy dome — tulip trees have a broad oval crown shape
	local canopyCenterY = topPos.y + math.floor(canopyWidth * 0.05)
	local canopyCenter = g_ivec3.new(pos.x, canopyCenterY, pos.z)
	local cW = canopyWidth
	-- Oval: taller than wide proportionally
	local cH = math.max(3, math.floor(canopyWidth * 0.55))
	g_shape.dome(volume, canopyCenter, 'y', false, cW * 2, cH, cW * 2, leafColor)
	-- Top accent layer
	local topCap = g_ivec3.new(pos.x, canopyCenterY + 1, pos.z)
	g_shape.dome(volume, topCap, 'y', false, math.floor(cW * 1.2), math.max(1, cH - 1),
		math.floor(cW * 1.2), leafColor2)
	-- Under-fill
	g_shape.dome(volume, canopyCenter, 'y', true,
		math.floor(cW * 0.7), math.max(1, cH - 2), math.floor(cW * 0.7), leafColor)

	-- Scatter a few flowers on top of the canopy
	if flowers then
		local flowerCount = math.random(3, 6 + math.floor(canopyWidth * 0.3))
		for _ = 1, flowerCount do
			local fx = pos.x + math.random(-cW + 1, cW - 1)
			local fz = pos.z + math.random(-cW + 1, cW - 1)
			local dist = math.sqrt((fx - pos.x) ^ 2 + (fz - pos.z) ^ 2)
			if dist <= cW - 1 then
				local fy = canopyCenterY + math.random(math.floor(cH * 0.3), cH)
				placeFlower(volume, g_ivec3.new(fx, fy, fz), flowerColor1, flowerColor2)
			end
		end
	end

	-- Leader tip at the very top
	local tipPos = g_ivec3.new(pos.x, topPos.y + cH + 1, pos.z)
	g_shape.line(volume, topPos, tipPos, leafColor, 1)
	leafCluster(volume, tipPos, 3, leafColor, leafColor2)
end
