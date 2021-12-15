local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'newcolor', desc = 'the palette color index', type = 'colorindex' }
	}
end

function main(volume, region, color, newcolor)
	local visitor = function (volume, x, y, z)
		volume:setVoxel(x, y, z, newcolor)
	end

	local condition = function (volume, x, y, z)
		local voxel = volume:voxel(x, y, z)
		if voxel == color then
			return true
		end
		return false
	end
	vol.conditionYXZ(volume, region, visitor, condition)
end
