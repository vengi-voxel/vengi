--
-- American Elm tree generator
-- Creates a tall American Elm with its characteristic vase-shaped silhouette:
-- a single straight trunk that fans out into arching limbs forming a broad,
-- graceful umbrella canopy.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '20', min = '8', max = '40' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'forkHeight', desc = 'Height fraction where trunk fans out (%)', type = 'int', default = '45', min = '25', max = '65' },
		{ name = 'mainBranches', desc = 'Number of main vase limbs', type = 'int', default = '6', min = '4', max = '10' },
		{ name = 'branchLength', desc = 'Length of main arching limbs', type = 'int', default = '12', min = '5', max = '22' },
		{ name = 'archHeight', desc = 'How high branches arch upward', type = 'int', default = '8', min = '3', max = '16' },
		{ name = 'subBranches', desc = 'Sub-branches per main limb', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'canopyDensity', desc = 'Foliage density (1=sparse, 5=very dense)', type = 'int', default = '3', min = '1', max = '5' },
		{ name = 'droopingTips', desc = 'Branch tips droop downward (elm characteristic)', type = 'bool', default = 'true' },
		{ name = 'trunkColor', desc = 'Bark color', type = 'hexcolor', default = '#6B5B4F' },
		{ name = 'branchColor', desc = 'Branch color', type = 'hexcolor', default = '#7D6B5D' },
		{ name = 'leafColor', desc = 'Primary leaf color', type = 'hexcolor', default = '#3D7A38' },
		{ name = 'leafColor2', desc = 'Secondary leaf color', type = 'hexcolor', default = '#5A9E4B' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates an American Elm with a tall vase-shaped silhouette, arching limbs and broad umbrella canopy'
end

local drawBezier = tree_utils.drawBezier

local leafCluster = tree_utils.leafCluster

-- Create an arching vase limb with sub-branches and drooping tips
local function createVaseLimb(volume, origin, angle, length, archH, branchColor,
	leafColor, leafColor2, subBranches, canopyDensity, droopingTips)

	local dx = math.cos(angle)
	local dz = math.sin(angle)

	-- Elm limbs grow steeply upward then arch outward — vase shape
	local endX = math.floor(origin.x + dx * length)
	local endZ = math.floor(origin.z + dz * length)
	local endY = origin.y + archH + math.random(-1, 1)

	local branchEnd = g_ivec3.new(endX, endY, endZ)

	-- Control point: steep initial rise then outward spread
	local ctrlX = math.floor(origin.x + dx * length * 0.25)
	local ctrlZ = math.floor(origin.z + dz * length * 0.25)
	local ctrlY = origin.y + archH + math.random(2, 4)
	local ctrl = g_ivec3.new(ctrlX, ctrlY, ctrlZ)

	local branchThick = math.max(1, math.floor(length * 0.15) + 1)
	local tip = drawBezier(volume, origin, branchEnd, ctrl, branchThick, 1,
		math.max(8, length), branchColor)

	-- Drooping tip extension — classic elm feature
	if droopingTips then
		local droopLen = math.random(2, math.max(3, math.floor(length * 0.35)))
		local droopEnd = g_ivec3.new(
			tip.x + math.floor(dx * droopLen * 0.5),
			tip.y - droopLen,
			tip.z + math.floor(dz * droopLen * 0.5)
		)
		g_shape.line(volume, tip, droopEnd, branchColor, 1)
		-- Foliage at droop tip
		leafCluster(volume, droopEnd, math.max(2, math.floor(length * 0.3)), leafColor, leafColor2)
	end

	-- Foliage at the main tip
	leafCluster(volume, tip, math.max(3, math.floor(length * 0.4)), leafColor, leafColor2)

	-- Sub-branches radiating from along the limb
	for s = 1, subBranches do
		local subT = 0.35 + (s - 1) * (0.5 / math.max(1, subBranches - 1))
		if subT > 0.9 then subT = 0.9 end

		local subOrigin = tree_utils.bezierPointAt(origin, branchEnd, ctrl, subT)

		local subAngle = angle + (math.random() - 0.5) * 1.8
		local subDx = math.cos(subAngle)
		local subDz = math.sin(subAngle)
		local subLen = math.max(2, math.floor(length * (0.3 + math.random() * 0.2)))
		local subRise = math.random(0, 3)

		local subEnd = g_ivec3.new(
			math.floor(subOrigin.x + subDx * subLen),
			subOrigin.y + subRise,
			math.floor(subOrigin.z + subDz * subLen)
		)
		local subCtrl = g_ivec3.new(
			math.floor(subOrigin.x + subDx * subLen * 0.4),
			subOrigin.y + subRise + math.random(1, 3),
			math.floor(subOrigin.z + subDz * subLen * 0.4)
		)
		drawBezier(volume, subOrigin, subEnd, subCtrl, 1, 1, math.max(5, subLen), branchColor)

		-- Foliage at sub-branch tip
		leafCluster(volume, subEnd, math.max(2, math.floor(subLen * 0.5)), leafColor, leafColor2)

		-- Drooping twig at sub-branch end
		if droopingTips and math.random() > 0.4 then
			local dEnd = g_ivec3.new(
				subEnd.x + math.random(-1, 1),
				subEnd.y - math.random(2, 4),
				subEnd.z + math.random(-1, 1)
			)
			g_shape.line(volume, subEnd, dEnd, branchColor, 1)
			leafCluster(volume, dEnd, math.max(1, math.floor(subLen * 0.3)), leafColor, leafColor2)
		end

		-- Extra foliage for dense canopy
		if canopyDensity >= 3 then
			local midPos = g_ivec3.new(
				math.floor((subOrigin.x + subEnd.x) / 2),
				math.floor((subOrigin.y + subEnd.y) / 2) + 1,
				math.floor((subOrigin.z + subEnd.z) / 2)
			)
			leafCluster(volume, midPos, math.max(2, math.floor(subLen * 0.35)), leafColor, leafColor2)
		end
	end

	-- Extra foliage clusters along the main limb for density
	if canopyDensity >= 2 then
		local numExtra = canopyDensity - 1
		for e = 1, numExtra do
			local eT = 0.3 + e * (0.5 / numExtra)
			local ePos = tree_utils.bezierPointAt(origin, branchEnd, ctrl, eT)
			ePos.y = ePos.y + 1
			leafCluster(volume, ePos, math.max(2, math.floor(length * 0.3)), leafColor, leafColor2)
		end
	end
end

function main(node, region, color, trunkHeight, trunkStrength, forkHeight,
	mainBranches, branchLength, archHeight, subBranches, canopyDensity, droopingTips,
	trunkColor, branchColor, leafColor, leafColor2, seed)

	tree_utils.initSeed(seed)

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Tall straight trunk — American Elms have notably straight trunks
	tree_utils.createTrunk(volume, pos, trunkHeight, trunkStrength, trunkColor)

	-- Root flare
	tree_utils.createBaseFlare(volume, pos, trunkStrength + 2, math.max(1, trunkStrength), trunkColor)

	-- Buttress roots
	tree_utils.createLineRoots(volume, pos, math.random(3, 5), trunkStrength + 1,
		math.max(1, trunkStrength - 1), trunkColor)

	-- Fork zone — where the trunk fans out into vase limbs
	local forkY = pos.y + math.floor(trunkHeight * forkHeight / 100)
	local forkPos = g_ivec3.new(pos.x, forkY, pos.z)

	-- Continue trunk slightly above fork
	local topPos = g_ivec3.new(pos.x, pos.y + trunkHeight, pos.z)
	local topThick = math.max(1, trunkStrength - 1)
	g_shape.line(volume, forkPos, topPos, trunkColor, topThick)

	-- Vase limbs radiating from fork zone — the defining elm silhouette
	local angleStep = (2 * math.pi) / mainBranches
	local startAngle = math.random() * 2 * math.pi

	for i = 1, mainBranches do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.3
		local bLen = branchLength + math.random(-2, 2)

		-- Vary origin height slightly around the fork zone
		local originY = forkY + math.random(-1, 2)
		local branchOrigin = g_ivec3.new(pos.x, originY, pos.z)

		createVaseLimb(volume, branchOrigin, angle, math.max(4, bLen), archHeight,
			branchColor, leafColor, leafColor2, subBranches, canopyDensity, droopingTips)
	end

	-- A couple of limbs from the top of the trunk as well
	for _ = 1, math.random(1, 2) do
		local topAngle = math.random() * 2 * math.pi
		local topLen = math.max(3, branchLength - math.random(2, 4))
		createVaseLimb(volume, topPos, topAngle, topLen, math.max(2, archHeight - 2),
			branchColor, leafColor, leafColor2, math.max(1, subBranches - 1), canopyDensity, droopingTips)
	end

	-- Umbrella canopy fill at the top
	local canopyY = forkY + archHeight + math.floor(archHeight * 0.3)
	local canopyCenter = g_ivec3.new(pos.x, canopyY, pos.z)
	local cW = math.floor(branchLength * 0.8)
	local cH = math.max(2, math.floor(archHeight * 0.35))
	tree_utils.createCanopyDome(volume, canopyCenter, cW, cH, leafColor, leafColor2, 1.3, 0)
end
