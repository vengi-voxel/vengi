--
-- build grass on surfaces (a surface is a voxel that doesn't have a voxel above it)
-- Select the color of the surface to build the grass on - all other voxel colors are
-- ignored. This allows you to e.g. place grass on the green voxels, but not on the
-- gray voxels that reprensent rocks or the street.
--

local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'grasscolor', desc = 'the color to use as grass', type = 'hexcolor', default = '#00FF00' },
		{ name = 'height', desc = 'the height of the grass to add', type = 'int', default = '4' },
		{ name = 'density', desc = 'the density of the grass', type = 'int', default = '2', min = '1', max = '100' },
		{ name = 'similarcolors', desc = 'the amount of similar colors', type = 'int', default = '4' },
		{ name = 'upwards', desc = 'let the grass grow upwards if true, downwards if false', type = 'bool', default = 'true' }
	}
end

function description()
	return "Adds grass to the surface of a given region. Select the color of the surface to build the grass on - all other voxel colors are ignored. This allows you to e.g. place grass on the green voxels, but not on the gray voxels that reprensent rocks or the street."
end

function main(node, region, color, grasscolor, height, density, similarcolors, upwards)
	local newindices = node:palette():similar(grasscolor, similarcolors)

	local visitor = function (volume, x, y, z)
		local rndHeight = math.random(1, height)
		local indidx = math.random(1, #newindices)
		local c = newindices[indidx]
		for h = 1, rndHeight do
			if upwards then
				if volume:voxel(x, y + h + 1, z) == -1 then
					volume:setVoxel(x, y + h, z, c)
				end
			else
				if volume:voxel(x, y - h - 1, z) == -1 then
					volume:setVoxel(x, y - h, z, c)
				end
			end
		end
	end

	local condition = function (volume, x, y, z)
		local d = math.random(1, density)
		if d ~= 1 then
			return false
		end
		local voxel = volume:voxel(x, y, z)
		if voxel == color then
			return true
		end
		return false
	end
	vol.conditionYXZDown(node:volume(), region, visitor, condition)
end
