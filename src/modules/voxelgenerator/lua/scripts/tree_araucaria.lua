--
-- Monkey Puzzle Tree (Araucaria araucana / Affenbaum) generator
-- Creates the distinctive silhouette: straight columnar trunk, whorled
-- branches in evenly spaced tiers, branches curve upward at tips, and
-- dense overlapping scale-like foliage covering each branch.
--

local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'height', desc = 'Total tree height', type = 'int', default = '30', min = '12', max = '60' },
		{ name = 'trunkStrength', desc = 'Trunk thickness', type = 'int', default = '2', min = '1', max = '5' },
		{ name = 'tiers', desc = 'Number of branch tiers', type = 'int', default = '6', min = '3', max = '12' },
		{ name = 'branchesPerTier', desc = 'Branches per whorl', type = 'int', default = '5', min = '3', max = '8' },
		{ name = 'branchLength', desc = 'Base branch length', type = 'int', default = '8', min = '3', max = '16' },
		{ name = 'foliageDensity', desc = 'Foliage cluster size', type = 'int', default = '3', min = '1', max = '6' },
		{ name = 'crownDome', desc = 'Add dome-shaped crown at top', type = 'bool', default = 'true' },
		{ name = 'trunkColor', desc = 'Trunk / bark color', type = 'hexcolor', default = '#6B4226' },
		{ name = 'leafColor', desc = 'Primary leaf color (dark green)', type = 'hexcolor', default = '#2F4F2F' },
		{ name = 'leafColor2', desc = 'Secondary leaf color', type = 'hexcolor', default = '#3B5E3B' },
		{ name = 'seed', desc = 'Random seed (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Creates a monkey puzzle tree (Araucaria) with whorled branch tiers and scale-like foliage'
end

-- Place a foliage cluster (small dome of overlapping scales)
local function foliageCluster(volume, pos, radius, leafColor, leafColor2)
	local col = leafColor
	if math.random() > 0.5 then
		col = leafColor2
	end
	if radius <= 1 then
		g_shape.line(volume, pos, pos, col, 1)
	else
		g_shape.dome(volume, pos, 'y', false, radius * 2, radius, radius * 2, col)
		-- Add a slight downward dome for the hanging scale-like look
		if radius >= 2 then
			g_shape.dome(volume, pos, 'y', true, math.floor(radius * 1.4), math.max(1, radius - 1), math.floor(radius * 1.4), col)
		end
	end
end

-- Draw foliage along a branch from start to tip
local function foliageBranch(volume, start, tip, foliageSize, leafColor, leafColor2)
	local dx = tip.x - start.x
	local dy = tip.y - start.y
	local dz = tip.z - start.z
	local dist = math.sqrt(dx * dx + dy * dy + dz * dz)
	local steps = math.max(2, math.floor(dist / math.max(1, foliageSize)))

	for i = 0, steps do
		local t = i / steps
		local fx = math.floor(start.x + dx * t)
		local fy = math.floor(start.y + dy * t)
		local fz = math.floor(start.z + dz * t)
		local pos = g_ivec3.new(fx, fy, fz)
		-- Clusters get slightly smaller toward the tip
		local r = math.max(1, math.floor(foliageSize * (1.0 - t * 0.3)))
		foliageCluster(volume, pos, r, leafColor, leafColor2)
	end
end

function main(node, region, color, height, trunkStrength, tiers, branchesPerTier,
	branchLength, foliageDensity, crownDome, trunkColor, leafColor, leafColor2, seed)

	if seed == 0 then
		math.randomseed(os.time())
	else
		math.randomseed(seed)
	end

	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	-- Straight trunk
	tree_utils.createTrunk(volume, pos, height, trunkStrength, trunkColor)

	-- Lower portion of the trunk is bare (typical for mature Araucaria)
	local bareHeight = math.floor(height * 0.25)

	-- Branch tiers — evenly spaced from bareHeight to near the top
	local tierSpacing = math.floor((height - bareHeight - 2) / math.max(1, tiers))

	for tier = 1, tiers do
		local tierY = pos.y + bareHeight + (tier - 1) * tierSpacing
		local tierFrac = (tier - 1) / math.max(1, tiers - 1)

		-- Lower tiers droop slightly, upper tiers curve upward (like real Araucaria)
		-- Branch length decreases toward the top for the pyramidal silhouette
		local tierLen = math.max(3, math.floor(branchLength * (1.0 - tierFrac * 0.5)))
		-- Rise: negative for lower tiers (drooping), positive for upper (upturned)
		local branchRise
		if tierFrac < 0.4 then
			branchRise = -math.floor(tierLen * 0.2)
		else
			branchRise = math.floor(tierLen * 0.3 * tierFrac)
		end

		-- Monkey puzzle branches curve upward at the tips
		local tipUpcurve = math.max(1, math.floor(tierLen * 0.4))

		local angleStep = (2 * math.pi) / branchesPerTier
		-- Stagger alternate tiers
		local startAngle = 0
		if tier % 2 == 1 then
			startAngle = angleStep * 0.5
		end

		for b = 1, branchesPerTier do
			local angle = startAngle + (b - 1) * angleStep + (math.random() - 0.5) * 0.15

			local dx = math.cos(angle)
			local dz = math.sin(angle)

			local branchStart = g_ivec3.new(pos.x, tierY, pos.z)

			-- Mid-point (slightly drooping or straight)
			local midX = math.floor(pos.x + dx * tierLen * 0.6)
			local midZ = math.floor(pos.z + dz * tierLen * 0.6)
			local midY = tierY + math.floor(branchRise * 0.5)

			-- Tip curves upward
			local tipX = math.floor(pos.x + dx * tierLen)
			local tipZ = math.floor(pos.z + dz * tierLen)
			local tipY = tierY + branchRise + tipUpcurve
			local tip = g_ivec3.new(tipX, tipY, tipZ)

			-- Draw branch wood
			local control = g_ivec3.new(midX, midY - 1, midZ)
			g_shape.bezier(volume, branchStart, tip, control, trunkColor, math.max(1, math.floor(trunkStrength * 0.6)))

			-- Foliage along the branch — the distinctive dense scale-like covering
			foliageBranch(volume, branchStart, tip, foliageDensity, leafColor, leafColor2)

			-- Extra cluster at the tip (branches often have a tuft at the end)
			foliageCluster(volume, tip, foliageDensity + 1, leafColor, leafColor2)
		end
	end

	-- Crown dome at top — mature Araucaria has a rounded crown
	if crownDome then
		local crownR = math.max(3, math.floor(branchLength * 0.5))
		local crownH = math.max(2, math.floor(crownR * 0.7))
		local crownPos = g_ivec3.new(pos.x, pos.y + height - 1, pos.z)
		g_shape.dome(volume, crownPos, 'y', false, crownR * 2, crownH, crownR * 2, leafColor)
		-- Accent layer
		g_shape.dome(volume, g_ivec3.new(crownPos.x, crownPos.y + 1, crownPos.z),
			'y', false, math.floor(crownR * 1.4), math.max(1, crownH - 1),
			math.floor(crownR * 1.4), leafColor2)
	end
end
