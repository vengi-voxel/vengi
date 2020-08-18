local perlin = require "modules.perlin"

function arguments()
	return {
		{ name = 'freq', desc = 'frequence for the noise function input', type = 'float', default = '0.3' },
		{ name = 'amplitude', desc = 'amplitude to scale the noise noise function output', type = 'float', default = '1.0' }
	}
end

function main(volume, region, color, freq, amplitude)
	perlin:load()
	local mins = region:mins()
	local maxs = region:maxs()
	for x = mins.x, maxs.x do
		for z = mins.z, maxs.z do
			local maxY = perlin:norm(amplitude * perlin:noise(x * freq, z * freq, freq)) * region:height()
			for y = 0, maxY do
				volume:setVoxel(x, y, z, color)
			end
		end
	end
end
