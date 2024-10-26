--
-- Smoothing of voxel edges.
--

local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'size', desc = 'The size of the smoothing.', type = 'int', default = '2', min = '1', max = '3' },
		{ name = 'strength', desc = 'The strength of the smoothing', type = 'float', default = '0.3', min = '0.0', max = '1.0' }
	}
end

function main(node, region, _, size, strength)
	local span = size * 2 + 1
	local cnt = span ^ 3

	local visitor = function(volume, x, y, z)
		volume:setVoxel(x, y, z, -1)
	end

	local condition = function(volume, x, y, z)
		if volume:voxel(x, y, z) == -1 then
			return false
		end
		local empty = vol.countEmptyAround(volume, x, y, z, size)
		return (cnt - empty) / cnt < strength
	end

	vol.conditionYXZ(node:volume(), region, visitor, condition)
end
