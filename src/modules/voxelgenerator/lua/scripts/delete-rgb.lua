--
-- deletes voxel by rgb values. it takes the closest palette index
--

local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'r', desc = 'red [0-255]', type = 'int', default = '0', min = '0', max = '255' },
		{ name = 'g', desc = 'green [0-255]', type = 'int', default = '0', min = '0', max = '255' },
		{ name = 'b', desc = 'blue [0-255]', type = 'int', default = '0', min = '0', max = '255' }
	}
end

function main(node, region, color, r, g, b)
	local newcolor = node:palette():match(r, g, b)

	local visitor = function (volume, x, y, z)
		volume:setVoxel(x, y, z, -1)
	end

	local condition = function (volume, x, y, z)
		local voxel = volume:voxel(x, y, z)
		if voxel == newcolor then
			return true
		end
		return false
	end
	vol.conditionYXZ(node:volume(), region, visitor, condition)
end
