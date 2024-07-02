--
-- moves all voxels of the current color into a new scene node. it takes the closest palette index
--

local vol = require "modules.volume"

function main(node, region, color)
	local newLayer = g_scenegraph.new('split', region)
	local newVolume = newLayer:volume()
	newLayer:setPalette(node:palette())

	local visitor = function (volume, x, y, z)
		newVolume:setVoxel(x, y, z, volume:voxel(x, y, z))
		volume:setVoxel(x, y, z, -1)
	end

	local condition = function (volume, x, y, z)
		local voxel = volume:voxel(x, y, z)
		if voxel == color then
			return true
		end
		return false
	end
	vol.conditionYXZ(node:volume(), region, visitor, condition)
	newVolume:crop()
end
