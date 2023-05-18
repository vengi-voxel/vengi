local perlin = require "modules.perlin"

function arguments()
	return {
		{ name = 'freq', desc = 'frequence for the noise function input', type = 'float', default = '0.3', min = '0.0', max = '5.0' },
		{ name = 'amplitude', desc = 'amplitude to scale the noise noise function output', type = 'float', default = '1.0' },
		{ name = 'offset', desc = 'the noise offset', type = 'float', default = '0.0' }
	}
end

function main(node, region, color, freq, amplitude, offset)
	local volume = node:volume()
	perlin:load()
	local mins = region:mins()
	local maxs = region:maxs()
	for x = mins.x, maxs.x do
		for z = mins.z, maxs.z do
			local maxY = perlin:norm(amplitude * perlin:noise(offset + x * freq, offset + z * freq, freq)) * region:height()
			for y = 0, maxY do
				volume:setVoxel(x, y, z, color)
			end
		end
	end
end
