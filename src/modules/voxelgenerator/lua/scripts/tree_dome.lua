local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '10' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '2' },
		{ name = 'leavesWidth', desc = 'Width of the leaves', type = 'int', default = '10' },
		{ name = 'leavesHeight', desc = 'Height of the leaves', type = 'int', default = '10' },
		{ name = 'leavesDepth', desc = 'Depth of the leaves', type = 'int', default = '10' },
		{ name = 'trunkColor', desc = 'Color of the trunk', type = 'colorindex', default = '1' },
		{ name = 'leavesColor', desc = 'Color of the leaves', type = 'colorindex', default = '2' }
	}
end

function main(node, region, color, trunkHeight, trunkStrength, leavesWidth, leavesHeight, leavesDepth, trunkColor, leavesColor)
	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	local trunkVoxel = trunkColor
	local leavesVoxel = leavesColor

	tree_utils.createTrunk(volume, pos, trunkHeight, trunkStrength, trunkVoxel)

	local leavesCenter = g_ivec3.new(pos.x, pos.y + trunkHeight + math.floor(leavesHeight / 2), pos.z)
	g_shape.dome(volume, leavesCenter, 'y', false, leavesWidth, leavesHeight, leavesDepth, leavesVoxel)
end
