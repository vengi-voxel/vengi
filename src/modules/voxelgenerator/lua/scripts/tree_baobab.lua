--
-- African Baobab Tree (Adansonia digitata / Affenbrotbaum) generator
-- Creates the iconic silhouette: massive bottle-shaped trunk, short stature
-- relative to girth, thick gnarled branches spreading outward like roots
-- (the "upside-down tree"), and sparse seasonal foliage clusters at branch tips.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'height', desc = 'Total tree height', type = 'int', default = '25', min = '10', max = '50' },
		{ name = 'trunkRadius', desc = 'Trunk radius (baobabs are very thick)', type = 'int', default = '6', min = '3', max = '14' },
		{ name = 'numBranches', desc = 'Number of main branches', type = 'int', default = '7', min = '4', max = '12' },
		{ name = 'branchLength', desc = 'Main branch length', type = 'int', default = '10', min = '4', max = '20' },
		{ name = 'subBranches', desc = 'Number of sub-branches per main branch', type = 'int', default = '3', min = '0', max = '6' },
		{ name = 'foliage', desc = 'Show foliage (baobabs are leafless most of the year)', type = 'bool', default = 'true' },
		{ name = 'foliageSize', desc = 'Foliage cluster size at branch tips', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'trunkColor', desc = 'Trunk bark color (smooth grey)', type = 'hexcolor', default = '#8B8378' },
		{ name = 'trunkColor2', desc = 'Trunk accent / variation', type = 'hexcolor', default = '#9C9085' },
		{ name = 'branchColor', desc = 'Branch color (lighter grey-brown)', type = 'hexcolor', default = '#7A7068' },
		{ name = 'leafColor', desc = 'Leaf color', type = 'hexcolor', default = '#4A7A3A' },
		{ name = 'leafColor2', desc = 'Leaf accent color', type = 'hexcolor', default = '#5C8C4C' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates an African baobab tree with a massive bottle-shaped trunk and gnarled spreading branches'
end

-- Build the massive bottle-shaped trunk using stacked ellipses
local function createBottleTrunk(volume, base, height, radius, trunkColor, trunkColor2)
	-- The trunk bulges in the middle and tapers at top and bottom
	for y = 0, height do
		local t = y / height
		-- Profile: wider in lower-middle, tapering at top
		local r
		if t < 0.1 then
			-- Base flare
			r = radius + math.floor((0.1 - t) * radius * 2)
		elseif t < 0.5 then
			-- Belly bulge
			local bulge = math.sin((t - 0.1) / 0.4 * math.pi) * radius * 0.3
			r = radius + math.floor(bulge)
		else
			-- Taper toward the top
			local taper = (t - 0.5) / 0.5
			r = math.max(2, math.floor(radius * (1.0 - taper * 0.6)))
		end

		local col = trunkColor
		-- Add color variation rings
		if math.random() > 0.7 then
			col = trunkColor2
		end

		local pos = g_ivec3.new(base.x, base.y + y, base.z)
		g_shape.ellipse(volume, pos, 'y', r * 2, 1, r * 2, col)
	end
end

-- Create a gnarled branch that forks and twists
local function createGnarledBranch(volume, start, angle, length, thickness, branchColor,
	subBranches, foliage, foliageSize, leafColor, leafColor2)

	local dx = math.cos(angle)
	local dz = math.sin(angle)

	-- Main branch rises slightly then goes mostly horizontal
	local rise = math.floor(length * (0.2 + math.random() * 0.2))
	local midX = math.floor(start.x + dx * length * 0.5)
	local midZ = math.floor(start.z + dz * length * 0.5)

	local tipX = math.floor(start.x + dx * length)
	local tipZ = math.floor(start.z + dz * length)
	local tipY = start.y + rise
	local tip = g_ivec3.new(tipX, tipY, tipZ)

	-- Control point for bezier — slight downward sag in middle
	local controlY = start.y + math.floor(rise * 0.3) - math.random(0, 2)
	local control = g_ivec3.new(midX, controlY, midZ)

	g_shape.bezier(volume, start, tip, control, branchColor, thickness)

	-- Foliage at the tip of the main branch
	if foliage then
		local col = leafColor
		if math.random() > 0.5 then col = leafColor2 end
		g_shape.dome(volume, tip, 'y', false,
			foliageSize * 2, foliageSize, foliageSize * 2, col)
	end

	-- Sub-branches forking off the main branch
	for s = 1, subBranches do
		local subT = 0.4 + (s / (subBranches + 1)) * 0.5
		local subStartX = math.floor(start.x + dx * length * subT)
		local subStartZ = math.floor(start.z + dz * length * subT)
		local subStartY = math.floor(start.y + rise * subT)
		local subStart = g_ivec3.new(subStartX, subStartY, subStartZ)

		-- Sub-branch angles diverge from main branch
		local subAngle = angle + (math.random() - 0.5) * 1.5
		local subLen = math.max(2, math.floor(length * (0.3 + math.random() * 0.3)))
		local subRise = math.floor(subLen * (0.3 + math.random() * 0.4))

		local subTipX = math.floor(subStartX + math.cos(subAngle) * subLen)
		local subTipZ = math.floor(subStartZ + math.sin(subAngle) * subLen)
		local subTipY = subStartY + subRise
		local subTip = g_ivec3.new(subTipX, subTipY, subTipZ)

		local subCtrl = g_ivec3.new(
			math.floor((subStartX + subTipX) / 2),
			subStartY + math.floor(subRise * 0.3),
			math.floor((subStartZ + subTipZ) / 2)
		)

		local subThick = math.max(1, thickness - 1)
		g_shape.bezier(volume, subStart, subTip, subCtrl, branchColor, subThick)

		-- Foliage at sub-branch tips
		if foliage then
			local col = leafColor
			if math.random() > 0.5 then col = leafColor2 end
			local fs = math.max(1, foliageSize - 1)
			g_shape.dome(volume, subTip, 'y', false, fs * 2, fs, fs * 2, col)
		end

		-- Occasional tertiary twig
		if math.random() > 0.5 then
			local twigAngle = subAngle + (math.random() - 0.5) * 1.2
			local twigLen = math.max(2, math.floor(subLen * 0.4))
			local twigTipX = math.floor(subTipX + math.cos(twigAngle) * twigLen)
			local twigTipZ = math.floor(subTipZ + math.sin(twigAngle) * twigLen)
			local twigTipY = subTipY + math.random(1, math.max(1, math.floor(twigLen * 0.5)))
			local twigTip = g_ivec3.new(twigTipX, twigTipY, twigTipZ)
			g_shape.line(volume, subTip, twigTip, branchColor, 1)

			if foliage and math.random() > 0.4 then
				local col = leafColor
				if math.random() > 0.5 then col = leafColor2 end
				local ts = math.max(1, foliageSize - 1)
				g_shape.dome(volume, twigTip, 'y', false, ts * 2, math.max(1, ts - 1), ts * 2, col)
			end
		end
	end
end

function main(node, region, color, height, trunkRadius, numBranches, branchLength,
	subBranches, foliage, foliageSize, trunkColor, trunkColor2, branchColor,
	leafColor, leafColor2, seed)

	tree_utils.initSeed(seed)

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Massive bottle-shaped trunk
	local trunkHeight = math.floor(height * 0.55)
	createBottleTrunk(volume, pos, trunkHeight, trunkRadius, trunkColor, trunkColor2)

	-- Branches emerge from the top of the trunk, spreading outward like roots
	local branchBaseY = pos.y + trunkHeight
	local angleStep = (2 * math.pi) / numBranches
	local startAngle = math.random() * 2 * math.pi

	for i = 1, numBranches do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.3
		local branchY = branchBaseY - math.random(0, math.max(0, math.floor(trunkHeight * 0.1)))
		local branchStart = g_ivec3.new(pos.x, branchY, pos.z)

		-- Vary branch length slightly
		local len = branchLength + math.random(-2, 2)
		local thick = math.max(1, math.floor(trunkRadius * 0.3))

		createGnarledBranch(volume, branchStart, angle, len, thick, branchColor,
			subBranches, foliage, foliageSize, leafColor, leafColor2)
	end

	-- A few upward branches near the center for the crown silhouette
	local centerBranches = math.random(2, 4)
	for _ = 1, centerBranches do
		local cAngle = math.random() * 2 * math.pi
		local cLen = math.max(3, math.floor(branchLength * 0.5))
		local cStart = g_ivec3.new(pos.x, branchBaseY, pos.z)
		local cTipX = math.floor(pos.x + math.cos(cAngle) * cLen * 0.3)
		local cTipZ = math.floor(pos.z + math.sin(cAngle) * cLen * 0.3)
		local cTipY = branchBaseY + cLen
		local cTip = g_ivec3.new(cTipX, cTipY, cTipZ)
		local cCtrl = g_ivec3.new(
			math.floor((pos.x + cTipX) / 2),
			branchBaseY + math.floor(cLen * 0.4),
			math.floor((pos.z + cTipZ) / 2)
		)
		g_shape.bezier(volume, cStart, cTip, cCtrl, branchColor, math.max(1, math.floor(trunkRadius * 0.25)))

		if foliage then
			local col = leafColor
			if math.random() > 0.5 then col = leafColor2 end
			g_shape.dome(volume, cTip, 'y', false,
				foliageSize * 2, foliageSize, foliageSize * 2, col)
		end
	end
end
