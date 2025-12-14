local tree_utils = require "modules.tree_utils"

function arguments()
	return {
		{ name = 'trunkHeight', desc = 'Height of the trunk', type = 'int', default = '20' },
		{ name = 'trunkStrength', desc = 'Thickness of the trunk', type = 'int', default = '2' },
		{ name = 'trunkWidth', desc = 'Width shift of trunk', type = 'int', default = '0' },
		{ name = 'trunkDepth', desc = 'Depth shift of trunk', type = 'int', default = '0' },
		{ name = 'trunkControlOffset', desc = 'Control point offset for trunk', type = 'int', default = '5' },
		{ name = 'trunkFactor', desc = 'Trunk size factor', type = 'float', default = '0.9' },
		{ name = 'leavesWidth', desc = 'Width of the leaves', type = 'int', default = '10' },
		{ name = 'leavesHeight', desc = 'Height of the leaves', type = 'int', default = '10' },
		{ name = 'branches', desc = 'Number of branches', type = 'int', default = '5' },
		{ name = 'branchSize', desc = 'Size of branches', type = 'float', default = '1.0' },
		{ name = 'branchControlOffset', desc = 'Control point offset for branches', type = 'int', default = '5' },
		{ name = 'branchFactor', desc = 'Branch size factor', type = 'float', default = '0.9' },
		{ name = 'randomLeavesHeightOffset', desc = 'Random height offset for leaves', type = 'int', default = '2' },
		{ name = 'trunkColor', desc = 'Color of the trunk', type = 'colorindex', default = '1' },
		{ name = 'leavesColor', desc = 'Color of the leaves', type = 'colorindex', default = '2' },
		{ name = 'seed', desc = 'Random seed', type = 'int', default = '0' }
	}
end

function main(node, region, color, trunkHeight, trunkStrength, trunkWidth, trunkDepth, trunkControlOffset, trunkFactor, leavesWidth, leavesHeight, branches, branchSize, branchControlOffset, branchFactor, randomLeavesHeightOffset, trunkColor, leavesColor, seed)
	math.randomseed(seed)
	local volume = node:volume()
	local pos = tree_utils.getCenterBottom(region)

	local trunkVoxel = trunkColor
	local leavesVoxel = leavesColor

	local startPos = tree_utils.createBezierTrunk(volume, pos, trunkHeight, trunkWidth, trunkDepth, trunkStrength, trunkControlOffset, trunkFactor, trunkVoxel)

	local stepWidth = (2 * math.pi) / branches
	local angle = math.random() * 2 * math.pi
	local w = leavesWidth

	for _ = 1, branches do
		local currentBranchSize = branchSize
		local x = math.cos(angle)
		local z = math.sin(angle)
		local randomLength = math.random(leavesHeight, leavesHeight + randomLeavesHeightOffset)

		local control = g_ivec3.new(
			math.floor(startPos.x - x * (w / 2.0)),
			math.floor(startPos.y + branchControlOffset),
			math.floor(startPos.z - z * (w / 2.0))
		)
		local endPos = g_ivec3.new(
			math.floor(startPos.x - x * w),
			math.floor(startPos.y - randomLength),
			math.floor(startPos.z - z * w)
		)

		local steps = math.floor(leavesHeight / 4)
		local last = startPos
		for i = 1, steps do
			local t = i / steps
			local invT = 1.0 - t
			local p = g_ivec3.new(
				math.floor(invT * invT * startPos.x + 2 * invT * t * control.x + t * t * endPos.x),
				math.floor(invT * invT * startPos.y + 2 * invT * t * control.y + t * t * endPos.y),
				math.floor(invT * invT * startPos.z + 2 * invT * t * control.z + t * t * endPos.z)
			)
			g_shape.line(volume, p, last, leavesVoxel, math.max(1, math.ceil(currentBranchSize)))
			currentBranchSize = currentBranchSize * branchFactor
			last = p
		end

		angle = angle + stepWidth
	end
end
