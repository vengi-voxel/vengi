local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '10' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '2' },
		{ name = 'leavesWidth', desc = 'Width of the leaves', type = 'int', default = '10' },
		{ name = 'leavesHeight', desc = 'Height of the leaves', type = 'int', default = '10' },
		{ name = 'leavesDepth', desc = 'Depth of the leaves', type = 'int', default = '10' },
		{ name = 'branches', desc = 'Number of hanging branches', type = 'int', default = '10' },
		{ name = 'hangingLeavesLengthMin', desc = 'Min length of hanging leaves', type = 'int', default = '5' },
		{ name = 'hangingLeavesLengthMax', desc = 'Max length of hanging leaves', type = 'int', default = '10' },
		{ name = 'hangingLeavesThickness', desc = 'Thickness of hanging leaves', type = 'int', default = '1' },
		{ name = 'trunkColor', desc = 'Color of the trunk', type = 'hexcolor', default = '#8B4513' },
		{ name = 'leavesColor', desc = 'Color of the leaves', type = 'hexcolor', default = '#2E6B14' },
		{ name = 'seed', desc = 'Random seed', type = 'int', default = '0' }
	}
end

function main(node, region, color, trunkHeight, trunkStrength, leavesWidth, leavesHeight, leavesDepth, branches, hangingLeavesLengthMin, hangingLeavesLengthMax, hangingLeavesThickness, trunkColor, leavesColor, seed)
	math.randomseed(seed)
	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	local trunkVoxel = trunkColor
	local leavesVoxel = leavesColor

	tree_utils.createTrunk(volume, pos, trunkHeight, trunkStrength, trunkVoxel)

	local leavesCenter = g_ivec3.new(pos.x, pos.y + trunkHeight, pos.z)
	g_shape.dome(volume, leavesCenter, 'y', false, leavesWidth, leavesHeight, leavesDepth, leavesVoxel)

	local stepWidth = (2 * math.pi) / branches
	local angle = math.random() * 2 * math.pi
	local y = pos.y + trunkHeight + 1

	for _ = 1, branches do
		local x = math.cos(angle)
		local z = math.sin(angle)
		local randomLength = math.random(hangingLeavesLengthMin, hangingLeavesLengthMax)

		local startX = math.floor(pos.x - x * (leavesWidth - 1) / 2.0 + 0.5)
		local startZ = math.floor(pos.z - z * (leavesDepth - 1) / 2.0 + 0.5)
		local start = g_ivec3.new(startX, y, startZ)
		local endPos = g_ivec3.new(startX, y - randomLength, startZ)

		g_shape.line(volume, start, endPos, leavesVoxel, hangingLeavesThickness)

		angle = angle + stepWidth
	end
end
