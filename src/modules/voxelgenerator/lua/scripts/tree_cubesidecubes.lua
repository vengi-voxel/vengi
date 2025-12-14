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

	g_shape.cube(volume, leavesCenter, leavesWidth, leavesHeight, leavesDepth, leavesVoxel)
	g_shape.cube(volume, leavesCenter, leavesWidth + 2, leavesHeight - 2, leavesDepth - 2, leavesVoxel)
	g_shape.cube(volume, leavesCenter, leavesWidth - 2, leavesHeight + 2, leavesDepth - 2, leavesVoxel)
	g_shape.cube(volume, leavesCenter, leavesWidth - 2, leavesHeight - 2, leavesDepth + 2, leavesVoxel)

	local halfWidth = math.floor(leavesWidth / 2)
	local halfHeight = math.floor(leavesHeight / 2)
	local halfDepth = math.floor(leavesDepth / 2)

	local offsets = {
		{x = 1, z = 0},
		{x = 0, z = 1},
		{x = -1, z = 0},
		{x = 0, z = -1}
	}

	for _, offset in ipairs(offsets) do
		local leavesPos2 = g_ivec3.new(leavesCenter.x, leavesCenter.y, leavesCenter.z)
		leavesPos2.x = leavesPos2.x + offset.x * halfWidth
		leavesPos2.z = leavesPos2.z + offset.z * halfDepth
		g_shape.ellipse(volume, leavesPos2, 'y', halfWidth, halfHeight, halfDepth, leavesVoxel)
	end
end
