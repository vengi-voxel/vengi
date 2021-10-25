local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'freq', desc = 'frequence for the noise function input', type = 'float', default = '0.05' },
		{ name = 'amplitude', desc = 'amplitude to scale the noise noise function output', type = 'float', default = '0.3' },
		{ name = 'dimensions', desc = 'the dimensions of the noise call', type = 'int', min = '2', max = '3', default = '2' },
		{ name = 'threshold', desc = 'the 3d noise threshold to place a voxel', type = 'float', min = '0.0', max = '1.0', default = '0.15' }
	}
end

function main(volume, region, color, freq, amplitude, dimensions, threshold)
	if (dimensions == 2) then
		local visitor = function (volume, x, z)
			local maxY = amplitude * noise.noise3(vec3.new(x * freq, z * freq, freq)) * region:height()
			for y = 0, maxY do
				volume:setVoxel(x, y, z, color)
			end
		end
		vol.visitXZ(volume, region, visitor)
	else
		local visitor = function (volume, x, y, z)
			local v = amplitude * noise.noise3(vec3.new(x * freq, y * freq, z * freq))
			if (v > threshold) then
				volume:setVoxel(x, y, z, color)
			end
		end

		vol.visitYXZ(volume, region, visitor)
	end
end
