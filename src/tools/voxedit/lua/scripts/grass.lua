local vol = require("modules.volume")

print (type(package.loaded["modules.volume"]))

print( type( vol ) )

function arguments()
	return {
		{ name = 'height', desc = 'the height of the grass to add', type = 'int', default = '4' },
		{ name = 'density', desc = 'the density of the grass', type = 'int', default = '2' },
		{ name = 'grasscolor', desc = 'the palette index of the color to use as grass', type = 'int' }
	}
end

function main(volume, region, color, height, density, grasscolor)
	local visitor = function (volume, x, y, z)
		for h = 1, height do
			volume:voxel(x, y + h, z, grasscolor)
		end
	end

	local condition = function (volume, x, y, z)
		local voxel = volume:voxel(x, y, z)
		if voxel == -grasscolor then
			return true
		end
		return false
	end
	vol.conditionXYZ(volume, region, visitor, condition)
end
