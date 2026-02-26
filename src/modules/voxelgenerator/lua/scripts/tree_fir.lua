local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '20' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '2' },
		{ name = 'branches', desc = 'Number of branches', type = 'int', default = '5' },
		{ name = 'amount', desc = 'Amount of branch layers', type = 'int', default = '5' },
		{ name = 'stepHeight', desc = 'Height step between layers', type = 'int', default = '2' },
		{ name = 'w', desc = 'Initial width', type = 'float', default = '5.0' },
		{ name = 'branchStrength', desc = 'Thickness of branches', type = 'int', default = '1' },
		{ name = 'branchDownwardOffset', desc = 'Downward offset for branch tip', type = 'int', default = '2' },
		{ name = 'branchPositionFactor', desc = 'Position factor for branch tip', type = 'float', default = '0.5' },
		{ name = 'trunkColor', desc = 'Color of the trunk', type = 'hexcolor', default = '#8B4513' },
		{ name = 'leavesColor', desc = 'Color of the leaves', type = 'hexcolor', default = '#1B4D1B' },
		{ name = 'seed', desc = 'Random seed', type = 'int', default = '0' }
	}
end

function main(node, region, color, trunkHeight, trunkStrength, branches, amount, stepHeight, w, branchStrength, branchDownwardOffset, branchPositionFactor, trunkColor, leavesColor, seed)
	math.randomseed(seed)
	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	local trunkVoxel = trunkColor
	local leavesVoxel = leavesColor

	tree_utils.createTrunk(volume, pos, trunkHeight, trunkStrength, trunkVoxel)

	local stepWidth = (2 * math.pi) / branches
	local angle = math.random() * 2 * math.pi
	local leavesPos = g_ivec3.new(pos.x, pos.y + trunkHeight, pos.z)

	-- Tip
	g_shape.line(volume, leavesPos, g_ivec3.new(leavesPos.x, leavesPos.y + 2, leavesPos.z), leavesVoxel, 1)

	local currentW = w
	for _ = 1, amount do
		for _ = 1, branches do
			local start = g_ivec3.new(leavesPos.x, leavesPos.y, leavesPos.z)
			local x = math.cos(angle)
			local z = math.sin(angle)

			if trunkStrength % 2 == 0 then
				if x > 0 then start.x = start.x - 1 end
				if z > 0 then start.z = start.z - 1 end
			end

			-- First segment: Outwards to the "bend" point
			local bendDist = currentW * branchPositionFactor
			local bendPos = g_ivec3.new(
				math.floor(start.x - x * bendDist),
				start.y - 1,
				math.floor(start.z - z * bendDist)
			)
			g_shape.line(volume, start, bendPos, leavesVoxel, branchStrength)

			-- Second segment: Downwards and outwards to the tip
			local tipDist = currentW
			local tipPos = g_ivec3.new(
				math.floor(start.x - x * tipDist),
				bendPos.y - branchDownwardOffset - math.random(0, 2),
				math.floor(start.z - z * tipDist)
			)
			g_shape.line(volume, bendPos, tipPos, leavesVoxel, branchStrength)

			angle = angle + stepWidth
		end
		currentW = currentW + 1.0
		leavesPos.y = leavesPos.y - stepHeight
		angle = angle + (stepWidth / 2)
	end
end
