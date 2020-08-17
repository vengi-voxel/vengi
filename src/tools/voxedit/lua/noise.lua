local perlin = require "modules.perlin"

function main(volume, region, color)
	perlin:load()
	local mins = region:mins()
	local maxs = region:maxs()
	for x = mins.x, maxs.x do
		for z = mins.z, maxs.z do
			local maxY = perlin:norm(perlin:noise(x / 10, z / 10, 0.3)) * region:height()
			for y = 0, maxY do
				volume:setVoxel(x, y, z, color)
			end
		end
	end
end
