local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'gridcolor', desc = 'the color for the grid stripes', type = 'colorindex' },
		{ name = 'thickness', desc = 'the color for the non stripes part', type = 'int', default = '1', min = '1' },
		{ name = 'size_x', desc = 'the x size of the grid', type = 'int', default = '5', min = '2' },
		{ name = 'size_y', desc = 'the y size of the grid', type = 'int', default = '5', min = '2' },
		{ name = 'size_z', desc = 'the z size of the grid', type = 'int', default = '5', min = '2' },
	}
end

function main(node, region, color, gridcolor, thickness, size_x, size_y, size_z)
	local visitor = function (volume, x, y, z)
		if size_x > thickness and x % size_x < thickness then
			volume:setVoxel(x, y, z, gridcolor)
		elseif size_y > thickness and y % size_y < thickness then
			volume:setVoxel(x, y, z, gridcolor)
		elseif size_z > thickness and z % size_z < thickness then
			volume:setVoxel(x, y, z, gridcolor)
		else
			volume:setVoxel(x, y, z, color)
		end
	end
	vol.visitYXZ(node:volume(), region, visitor)
end
