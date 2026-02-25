--
-- Bonsai tree generator
-- Creates a stylized bonsai tree with a curved trunk, gnarled branches,
-- flat cloud-like foliage clusters, surface roots, and an optional pot.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '14', min = '6', max = '40' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk base', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'trunkCurve', desc = 'Horizontal curve offset of the trunk', type = 'int', default = '4', min = '0', max = '15' },
		{ name = 'branches', desc = 'Number of main foliage branches', type = 'int', default = '5', min = '2', max = '8' },
		{ name = 'branchLength', desc = 'Length of branches', type = 'int', default = '6', min = '2', max = '15' },
		{ name = 'foliageRadius', desc = 'Radius of foliage clusters', type = 'int', default = '5', min = '2', max = '12' },
		{ name = 'foliageHeight', desc = 'Height (flatness) of foliage clusters', type = 'int', default = '3', min = '1', max = '8' },
		{ name = 'roots', desc = 'Number of visible surface roots', type = 'int', default = '4', min = '0', max = '8' },
		{ name = 'rootLength', desc = 'Length of surface roots', type = 'int', default = '5', min = '2', max = '10' },
		{ name = 'pot', desc = 'Draw a pot at the base', type = 'bool', default = 'true' },
		{ name = 'potHeight', desc = 'Height of the pot', type = 'int', default = '3', min = '2', max = '6' },
		{ name = 'potRadius', desc = 'Radius of the pot', type = 'int', default = '8', min = '4', max = '15' },
		{ name = 'trunkColor', desc = 'Color of the trunk and branches', type = 'colorindex', default = '1' },
		{ name = 'leavesColor', desc = 'Color of the foliage', type = 'colorindex', default = '2' },
		{ name = 'potColor', desc = 'Color of the pot', type = 'colorindex', default = '3' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a bonsai tree with curved trunk, cloud-like foliage, surface roots and optional pot'
end

-- Draw a bezier curve between two points with a control point, tapering thickness
local function drawBezierSegment(volume, startPos, endPos, control, startThickness, endThickness, steps, voxelColor)
	local last = startPos
	for i = 1, steps do
		local t = i / steps
		local invT = 1.0 - t
		local p = g_ivec3.new(
			math.floor(invT * invT * startPos.x + 2 * invT * t * control.x + t * t * endPos.x),
			math.floor(invT * invT * startPos.y + 2 * invT * t * control.y + t * t * endPos.y),
			math.floor(invT * invT * startPos.z + 2 * invT * t * control.z + t * t * endPos.z)
		)
		local thickness = math.max(1, math.ceil(startThickness + (endThickness - startThickness) * t))
		g_shape.line(volume, last, p, voxelColor, thickness)
		last = p
	end
	return last
end

-- Create a flat, cloud-like foliage cluster (wide dome, short height)
local function createFoliageCluster(volume, center, radius, height, voxelColor)
	-- Main flat dome
	g_shape.dome(volume, center, 'y', false, radius * 2, height, radius * 2, voxelColor)
	-- Small inverted dome underneath for volume
	local underCenter = g_ivec3.new(center.x, center.y, center.z)
	local underHeight = math.max(1, math.floor(height / 2))
	g_shape.dome(volume, underCenter, 'y', true, math.floor(radius * 1.6), underHeight, math.floor(radius * 1.6), voxelColor)
end

-- Create surface roots spreading from the trunk base
local function createRoots(volume, basePos, numRoots, rootLength, trunkStrength, voxelColor)
	if numRoots <= 0 then
		return
	end
	local stepAngle = (2 * math.pi) / numRoots
	local startAngle = math.random() * 2 * math.pi
	for i = 1, numRoots do
		local angle = startAngle + (i - 1) * stepAngle + (math.random() - 0.5) * 0.5
		local dx = math.cos(angle)
		local dz = math.sin(angle)
		local startP = g_ivec3.new(basePos.x, basePos.y, basePos.z)
		local endP = g_ivec3.new(
			math.floor(basePos.x + dx * rootLength),
			basePos.y - math.random(0, 1),
			math.floor(basePos.z + dz * rootLength)
		)
		local ctrl = g_ivec3.new(
			math.floor(basePos.x + dx * rootLength * 0.5),
			basePos.y + math.random(0, 1),
			math.floor(basePos.z + dz * rootLength * 0.5)
		)
		local rootThickness = math.max(1, math.floor(trunkStrength * 0.6))
		drawBezierSegment(volume, startP, endP, ctrl, rootThickness, 1, math.max(4, rootLength), voxelColor)
	end
end

-- Create a pot (cylinder with rim)
local function createPot(volume, centerPos, potRadius, potHeight, voxelColor)
	-- Pot body - cylinder
	local bodyPos = g_ivec3.new(centerPos.x, centerPos.y, centerPos.z)
	local innerRadius = potRadius - 1
	g_shape.cylinder(volume, bodyPos, 'y', potRadius, potHeight, voxelColor)
	-- Rim at the top - slightly wider
	local rimPos = g_ivec3.new(centerPos.x, centerPos.y + potHeight - 1, centerPos.z)
	g_shape.cylinder(volume, rimPos, 'y', potRadius + 1, 1, voxelColor)
	-- Hollow out the inside (fill with air from one voxel above bottom)
	if innerRadius > 1 then
		local hollowPos = g_ivec3.new(centerPos.x, centerPos.y + 1, centerPos.z)
		g_shape.cylinder(volume, hollowPos, 'y', innerRadius, potHeight, -1)
	end
	-- Small foot/base
	local footPos = g_ivec3.new(centerPos.x, centerPos.y, centerPos.z)
	g_shape.cylinder(volume, footPos, 'y', math.max(2, potRadius - 2), 1, voxelColor)
end

function main(node, region, color, trunkHeight, trunkStrength, trunkCurve, branches,
	branchLength, foliageRadius, foliageHeight, roots, rootLength,
	pot, potHeight, potRadius, trunkColor, leavesColor, potColor, seed)

	if seed == 0 then
		math.randomseed(os.time())
	else
		math.randomseed(seed)
	end

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	local trunkVoxel = trunkColor
	local leavesVoxel = leavesColor
	local potVoxel = potColor

	-- Calculate base Y position (above pot if present)
	local baseY = pos.y
	if pot then
		-- Draw pot first
		local potCenter = g_ivec3.new(pos.x, pos.y, pos.z)
		createPot(volume, potCenter, potRadius, potHeight, potVoxel)
		-- Fill top of pot with soil (use trunk color as soil)
		local soilPos = g_ivec3.new(pos.x, pos.y + potHeight - 1, pos.z)
		local soilRadius = potRadius - 1
		if soilRadius > 1 then
			g_shape.cylinder(volume, soilPos, 'y', soilRadius, 1, trunkVoxel)
		end
		baseY = pos.y + potHeight
	end

	-- Trunk base position
	local trunkBase = g_ivec3.new(pos.x, baseY, pos.z)

	-- Create surface roots
	createRoots(volume, trunkBase, roots, rootLength, trunkStrength, trunkVoxel)

	-- Create curved trunk using bezier
	-- The trunk curves to one side (classic bonsai lean)
	local curveDir = math.random() * 2 * math.pi
	local curveX = math.floor(math.cos(curveDir) * trunkCurve)
	local curveZ = math.floor(math.sin(curveDir) * trunkCurve)

	local trunkTop = g_ivec3.new(
		trunkBase.x + curveX,
		trunkBase.y + trunkHeight,
		trunkBase.z + curveZ
	)
	local trunkControl = g_ivec3.new(
		trunkBase.x + math.floor(curveX * 0.3),
		trunkBase.y + math.floor(trunkHeight * 0.4),
		trunkBase.z + math.floor(curveZ * 0.3)
	)
	local topThickness = math.max(1, math.floor(trunkStrength * 0.5))
	drawBezierSegment(volume, trunkBase, trunkTop, trunkControl,
		trunkStrength, topThickness, trunkHeight, trunkVoxel)

	-- Add a slight bulge at the trunk base (nebari - surface root flare)
	local bulgePos = g_ivec3.new(trunkBase.x, trunkBase.y, trunkBase.z)
	local bulgeRadius = trunkStrength + 1
	g_shape.dome(volume, bulgePos, 'y', false, bulgeRadius * 2, math.max(1, math.floor(trunkStrength * 0.5)), bulgeRadius * 2, trunkVoxel)

	-- Create branches and foliage
	-- The branches should radiate outward at different heights and angles
	local stepAngle = (2 * math.pi) / branches
	local startAngle = math.random() * 2 * math.pi

	for i = 1, branches do
		local angle = startAngle + (i - 1) * stepAngle + (math.random() - 0.5) * 0.6

		-- Calculate branch start point along the trunk
		local branchT
		if i == branches then
			-- Last branch is at the very top (apex foliage)
			branchT = 1.0
		else
			-- Distribute branches along the upper portion of the trunk
			branchT = 0.4 + 0.55 * ((i - 1) / (branches - 1))
		end

		-- Interpolate position on the trunk curve
		local invT = 1.0 - branchT
		local branchOrigin = g_ivec3.new(
			math.floor(invT * invT * trunkBase.x + 2 * invT * branchT * trunkControl.x + branchT * branchT * trunkTop.x),
			math.floor(invT * invT * trunkBase.y + 2 * invT * branchT * trunkControl.y + branchT * branchT * trunkTop.y),
			math.floor(invT * invT * trunkBase.z + 2 * invT * branchT * trunkControl.z + branchT * branchT * trunkTop.z)
		)

		local dx = math.cos(angle)
		local dz = math.sin(angle)

		-- Branch length varies - top branches shorter, lower branches longer
		local thisLength
		if i == branches then
			thisLength = math.max(2, math.floor(branchLength * 0.4))
		else
			thisLength = branchLength + math.random(-1, 2)
		end

		-- Branch end point - slightly upward for bonsai style
		local branchEnd = g_ivec3.new(
			math.floor(branchOrigin.x + dx * thisLength),
			branchOrigin.y + math.random(0, 2),
			math.floor(branchOrigin.z + dz * thisLength)
		)

		-- Branch control point - slightly drooping then rising
		local branchCtrl = g_ivec3.new(
			math.floor(branchOrigin.x + dx * thisLength * 0.5),
			branchOrigin.y + math.random(-1, 1),
			math.floor(branchOrigin.z + dz * thisLength * 0.5)
		)

		local branchThickness = math.max(1, math.floor(topThickness * 0.8))
		local branchTip = drawBezierSegment(volume, branchOrigin, branchEnd, branchCtrl,
			branchThickness, 1, math.max(4, thisLength), trunkVoxel)

		-- Create foliage cluster at branch tip
		local thisFoliageRadius = foliageRadius + math.random(-1, 1)
		local thisFoliageHeight = foliageHeight + math.random(-1, 0)
		if i == branches then
			-- Apex foliage is slightly larger
			thisFoliageRadius = foliageRadius + 1
			thisFoliageHeight = foliageHeight + 1
		end
		thisFoliageRadius = math.max(2, thisFoliageRadius)
		thisFoliageHeight = math.max(1, thisFoliageHeight)
		createFoliageCluster(volume, branchTip, thisFoliageRadius, thisFoliageHeight, leavesVoxel)
	end
end
