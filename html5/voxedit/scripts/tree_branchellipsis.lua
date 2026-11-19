local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '10' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '2' },
		{ name = 'leavesWidth', desc = 'Width of the leaves', type = 'int', default = '10' },
		{ name = 'leavesHeight', desc = 'Height of the leaves', type = 'int', default = '10' },
		{ name = 'leavesDepth', desc = 'Depth of the leaves', type = 'int', default = '10' },
		{ name = 'branchHeight', desc = 'Height of branches', type = 'int', default = '5' },
		{ name = 'branchLength', desc = 'Length of branches', type = 'int', default = '5' },
		{ name = 'trunkColor', desc = 'Color of the trunk', type = 'hexcolor', default = '#8B4513' },
		{ name = 'leavesColor', desc = 'Color of the leaves', type = 'hexcolor', default = '#2E6B14' },
		{ name = 'seed', desc = 'Random seed', type = 'int', default = '0' }
	}
end

function main(node, region, color, trunkHeight, trunkStrength, leavesWidth, leavesHeight, leavesDepth, branchHeight, branchLength, trunkColor, leavesColor, seed)
	math.randomseed(seed)
	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	local trunkVoxel = trunkColor
	local leavesVoxel = leavesColor

	local top = pos.y + trunkHeight
	local basePos = g_ivec3.new(pos.x - 1, pos.y, pos.z - 1)
	g_shape.cube(volume, basePos, trunkStrength + 2, 1, trunkStrength + 2, trunkVoxel)
	g_shape.cube(volume, pos, trunkStrength, trunkHeight, trunkStrength, trunkVoxel)

	if trunkHeight > 8 then
		local branches = {1, 2, 3, 4}
		local n = math.random(1, 4)
		for i = n, n + 3 do
			local thickness = math.max(1, math.floor(trunkStrength / 2))

			local branch = g_ivec3.new(pos.x, math.random(pos.y + 2, top - 2), pos.z)
			local delta = math.floor((trunkStrength - thickness) / 2)
			local leavesPos

			local branchType = branches[(i - 1) % 4 + 1]
			if branchType == 1 then
				branch.x = branch.x + delta
				leavesPos = tree_utils.createL(volume, branch, 0, branchLength, branchHeight, thickness, trunkVoxel)
			elseif branchType == 2 then
				branch.x = branch.x + delta
				leavesPos = tree_utils.createL(volume, branch, 0, -branchLength, branchHeight, thickness, trunkVoxel)
			elseif branchType == 3 then
				branch.z = branch.z + delta
				leavesPos = tree_utils.createL(volume, branch, branchLength, 0, branchHeight, thickness, trunkVoxel)
			elseif branchType == 4 then
				branch.z = branch.z + delta
				leavesPos = tree_utils.createL(volume, branch, -branchLength, 0, branchHeight, thickness, trunkVoxel)
			end

			g_shape.ellipse(volume, leavesPos, 'y', branchHeight, branchHeight, branchHeight, leavesVoxel)
		end
	end

	local leavesPos = g_ivec3.new(pos.x + math.floor(trunkStrength / 2), top, pos.z + math.floor(trunkStrength / 2))
	g_shape.ellipse(volume, leavesPos, 'y', leavesWidth, leavesHeight, leavesDepth, leavesVoxel)
end
