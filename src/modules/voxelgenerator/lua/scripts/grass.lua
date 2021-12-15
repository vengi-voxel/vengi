local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'grasscolor', desc = 'the palette index of the color to use as grass', type = 'colorindex', default = '-1', min = '-1', max = '255' },
		{ name = 'height', desc = 'the height of the grass to add', type = 'int', default = '4' },
		{ name = 'density', desc = 'the density of the grass', type = 'int', default = '2' },
		{ name = 'similarcolors', desc = 'the amount of similar colors', type = 'int', default = '4' }
	}
end

function main(volume, region, color, grasscolor, height, density, similarcolors)
	if grasscolor == -1 then
		grasscolor = palette.match(0, 255, 0)
	end
	local newindices = palette.similar(grasscolor, similarcolors)

	local visitor = function (volume, x, y, z)
		local rndHeight = math.random(1, height)
		local indidx = math.random(1, #newindices)
		local c = newindices[indidx]
		for h = 1, rndHeight do
			if volume:voxel(x, y + h + 1, z) == -1 then
				volume:setVoxel(x, y + h, z, c)
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
	vol.conditionYXZDown(volume, region, visitor, condition)
end
