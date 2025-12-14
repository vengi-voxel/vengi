local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '20' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '2' },
		{ name = 'leavesWidth', desc = 'Width of the leaves', type = 'int', default = '10' },
		{ name = 'leavesHeight', desc = 'Height of the leaves', type = 'int', default = '10' },
		{ name = 'leavesDepth', desc = 'Depth of the leaves', type = 'int', default = '10' },
		{ name = 'singleLeafHeight', desc = 'Height of single leaf layer', type = 'int', default = '2' },
		{ name = 'singleStepDelta', desc = 'Step delta', type = 'int', default = '1' },
		{ name = 'startWidth', desc = 'Start width', type = 'int', default = '2' },
		{ name = 'startDepth', desc = 'Start depth', type = 'int', default = '2' },
		{ name = 'trunkColor', desc = 'Color of the trunk', type = 'colorindex', default = '1' },
		{ name = 'leavesColor', desc = 'Color of the leaves', type = 'colorindex', default = '2' },
		{ name = 'seed', desc = 'Random seed', type = 'int', default = '0' }
	}
end

function main(node, region, color, trunkHeight, trunkStrength, leavesWidth, leavesHeight, leavesDepth, singleLeafHeight, singleStepDelta, startWidth, startDepth, trunkColor, leavesColor, seed)
	math.randomseed(seed)
	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	local trunkVoxel = trunkColor
	local leavesVoxel = leavesColor

	tree_utils.createTrunk(volume, pos, trunkHeight, trunkStrength, trunkVoxel)

	local singleStepHeight = singleLeafHeight + singleStepDelta
	local steps = math.max(1, math.floor(leavesHeight / singleStepHeight))
	local stepWidth = math.floor(leavesWidth / steps)
	local stepDepth = math.floor(leavesDepth / steps)
	local currentWidth = startWidth
	local currentDepth = startDepth
	local top = pos.y + trunkHeight
	local leavesPos = g_ivec3.new(pos.x, top, pos.z)

	for _ = 1, steps do
		g_shape.dome(volume, leavesPos, 'y', false, currentWidth, singleLeafHeight, currentDepth, leavesVoxel)
		leavesPos.y = leavesPos.y - singleStepDelta
		g_shape.dome(volume, leavesPos, 'y', false, currentWidth + 1, singleLeafHeight, currentDepth + 1, leavesVoxel)
		currentDepth = currentDepth + stepDepth
		currentWidth = currentWidth + stepWidth
		leavesPos.y = leavesPos.y - singleLeafHeight
	end
end
