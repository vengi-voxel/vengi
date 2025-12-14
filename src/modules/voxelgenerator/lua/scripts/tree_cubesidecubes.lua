local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '10' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '2' },
		{ name = 'leavesWidth', desc = 'Width of the leaves', type = 'int', default = '10' },
		{ name = 'leavesHeight', desc = 'Height of the leaves', type = 'int', default = '10' },
		{ name = 'leavesDepth', desc = 'Depth of the leaves', type = 'int', default = '10' },
		{ name = 'trunkColor', desc = 'Color of the trunk', type = 'colorindex', default = '1' },
		{ name = 'leavesColor', desc = 'Color of the leaves', type = 'colorindex', default = '2' },
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

	local leavesPos = g_ivec3.new(
		pos.x - math.floor(leavesWidth / 2),
		pos.y + trunkHeight,
		pos.z - math.floor(leavesDepth / 2)
	)

	g_shape.cube(volume, leavesPos, leavesWidth, leavesHeight, leavesDepth, leavesVoxel)

	local leavesPos2 = g_ivec3.new(leavesPos.x, leavesPos.y + 1, leavesPos.z + 1)
	g_shape.cube(volume, leavesPos2, leavesWidth, leavesHeight - 2, leavesDepth - 2, leavesVoxel)

	local leavesPos3 = g_ivec3.new(leavesPos.x + 1, leavesPos.y, leavesPos.z + 1)
	g_shape.cube(volume, leavesPos3, leavesWidth - 2, leavesHeight, leavesDepth - 2, leavesVoxel)

	local leavesPos4 = g_ivec3.new(leavesPos.x + 1, leavesPos.y + 1, leavesPos.z)
	g_shape.cube(volume, leavesPos4, leavesWidth - 2, leavesHeight - 2, leavesDepth, leavesVoxel)

	local halfWidth = math.floor(leavesWidth / 2)
	local halfHeight = math.floor(leavesHeight / 2)
	local halfDepth = math.floor(leavesDepth / 2)

	local offsets = {
		{x = 1, z = 0},
		{x = 0, z = 1},
		{x = -1, z = 0},
		{x = 0, z = -1}
	}

	local leavesCenter = g_ivec3.new(pos.x, pos.y + trunkHeight, pos.z)
	for _, offset in ipairs(offsets) do
		local leavesPosSide = g_ivec3.new(leavesCenter.x, leavesCenter.y, leavesCenter.z)
		leavesPosSide.x = leavesPosSide.x + offset.x * halfWidth
		leavesPosSide.z = leavesPosSide.z + offset.z * halfDepth
		g_shape.ellipse(volume, leavesPosSide, 'y', halfWidth, halfHeight, halfDepth, leavesVoxel)
	end
end
