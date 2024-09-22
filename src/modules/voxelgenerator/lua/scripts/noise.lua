local perlin = require "modules.perlin"
local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'freq', desc = 'frequence for the noise function input', type = 'float', default = '0.3', min = '0.0', max = '5.0' },
		{ name = 'amplitude', desc = 'amplitude to scale the noise noise function output', type = 'float', default = '1.0' },
		{ name = 'offset', desc = 'the noise offset', type = 'float', default = '0.0' }
	}
end

function main(node, region, color, freq, amplitude, offset)
	perlin:load()

	local visitor = function (volume, x, z)
		local maxY = perlin:norm(amplitude * perlin:noise(offset + x * freq, offset + z * freq, freq)) * region:height()
		for y = 0, maxY do
			volume:setVoxel(x, y, z, color)
		end
	end
	vol.visitXZ(node:volume(), region, visitor)
end
