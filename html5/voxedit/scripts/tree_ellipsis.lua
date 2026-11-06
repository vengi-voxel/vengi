local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '8' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '2' },
		{ name = 'leavesWidth', desc = 'Width of the leaves', type = 'int', default = '12' },
		{ name = 'leavesHeight', desc = 'Height of the leaves', type = 'int', default = '12' },
		{ name = 'leavesDepth', desc = 'Depth of the leaves', type = 'int', default = '12' },
		{ name = 'trunkColor', desc = 'Color of the trunk', type = 'hexcolor', default = '#8B4513' },
		{ name = 'leavesColor', desc = 'Color of the leaves', type = 'hexcolor', default = '#2E6B14' },
		{ name = 'seed', desc = 'Random seed', type = 'int', default = '0' }
	}
end

function main(node, region, color, trunkHeight, trunkStrength, leavesWidth, leavesHeight, leavesDepth, trunkColor, leavesColor, seed)
	math.randomseed(seed)
	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	local trunkVoxel = trunkColor
	local leavesVoxel = leavesColor

	tree_utils.createTrunk(volume, pos, trunkHeight, trunkStrength, trunkVoxel)

	local leavesCenter = g_ivec3.new(pos.x, pos.y + trunkHeight, pos.z)
	g_shape.ellipse(volume, leavesCenter, 'y', leavesWidth, leavesHeight, leavesDepth, leavesVoxel)
end
