--
-- calculate erosion
--

local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'emptycnt', desc = 'The amount of empty voxels surrounding the voxel to erode.', type = 'int', default = '12', min = '1', max = '25' },
		{ name = 'octaves', type = 'int', default = '4', min = '1', max = '10' },
		{ name = 'lacunarity', desc = 'Scales the frequency', type = 'float', default = '1.0', min = '-3', max = '3'  },
		{ name = 'gain', desc = 'Scales the amplitude', type = 'float', default = '0.5', min = '-3', max = '3' },
		{ name = 'threshold', desc = 'Noise threshold', type = 'float', default = '0.3', min = '-1', max = '1' }
	}
end

function main(node, region, color, emptycnt, octaves, lacunarity, gain, threshold)
	local visitor = function (volume, x, y, z)
		local adjacent = vol.countEmptyAround(volume, x, y, z, 1)
		if (adjacent >= emptycnt) then
			local size = region:size()
			local p = g_vec3.new(x / size.x, y / size.y, z / size.z)
			local r = g_noise.fBm3(p, octaves, lacunarity, gain)
			if r >= threshold then
				volume:setVoxel(x, y, z, -1)
			end
		end
	end

	local condition = function (volume, x, y, z)
		local voxel = volume:voxel(x, y, z)
		if voxel == color then
			return true
		end
		return false
	end
	vol.conditionYXZ(node:volume(), region, visitor, condition)
end
