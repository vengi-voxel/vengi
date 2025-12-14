local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '10' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '2' },
		{ name = 'branches', desc = 'Number of branches', type = 'int', default = '5' },
		{ name = 'amount', desc = 'Amount of branch layers', type = 'int', default = '5' },
		{ name = 'stepHeight', desc = 'Height step between layers', type = 'int', default = '2' },
		{ name = 'w', desc = 'Initial width', type = 'float', default = '5.0' },
		{ name = 'branchStrength', desc = 'Thickness of branches', type = 'int', default = '1' },
		{ name = 'branchDownwardOffset', desc = 'Downward offset for branch tip', type = 'int', default = '2' },
		{ name = 'branchPositionFactor', desc = 'Position factor for branch tip', type = 'float', default = '0.5' },
		{ name = 'trunkColor', desc = 'Color of the trunk', type = 'colorindex', default = '1' },
		{ name = 'leavesColor', desc = 'Color of the leaves', type = 'colorindex', default = '2' }
	}
end

function main(node, region, color, trunkHeight, trunkStrength, branches, amount, stepHeight, w, branchStrength, branchDownwardOffset, branchPositionFactor, trunkColor, leavesColor)
	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	local trunkVoxel = trunkColor
	local leavesVoxel = leavesColor

	tree_utils.createTrunk(volume, pos, trunkHeight, trunkStrength, trunkVoxel)

	local stepWidth = (2 * math.pi) / branches
	local angle = math.random() * 2 * math.pi
	local leavesPos = g_ivec3.new(pos.x, pos.y + trunkHeight, pos.z)

	local halfHeight = math.floor(((amount - 1) * stepHeight) / 2)
	local center = g_ivec3.new(pos.x, pos.y + trunkHeight - halfHeight, pos.z)
	g_shape.cube(volume, center, trunkStrength, halfHeight * 2, trunkStrength, leavesVoxel)

	local currentW = w
	for _ = 1, amount do
		for b = 1, branches do
			local start = g_ivec3.new(leavesPos.x, leavesPos.y, leavesPos.z)
			local endPos = g_ivec3.new(start.x, start.y, start.z)

			local x = math.cos(angle)
			local z = math.sin(angle)
			local randomZ = math.random(4, 8)

			endPos.y = endPos.y - randomZ
			endPos.x = math.floor(endPos.x - x * currentW)
			endPos.z = math.floor(endPos.z - z * currentW)

			g_shape.line(volume, start, endPos, leavesVoxel, branchStrength)

			local end2 = g_ivec3.new(endPos.x, endPos.y, endPos.z)
			end2.y = end2.y - branchDownwardOffset
			end2.x = math.floor(end2.x - x * currentW * branchPositionFactor)
			end2.z = math.floor(end2.z - z * currentW * branchPositionFactor)

			g_shape.line(volume, endPos, end2, leavesVoxel, branchStrength)

			angle = angle + stepWidth
			currentW = currentW + (1.0 / (b + 1))
		end
		leavesPos.y = leavesPos.y - stepHeight
	end
end
