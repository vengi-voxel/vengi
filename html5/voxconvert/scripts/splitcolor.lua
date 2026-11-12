function arguments()
	return {
		{ name = 'exactmatch', desc = 'Only exact match, not similar colors', type = 'bool', default = 'true' }
	}
end

function description()
	return "Moves all voxels of the current color into a new scene node. It takes the closest palette index."
end

local vol = require "modules.volume"

function main(node, region, color, exactmatch)
	local newLayer = g_scenegraph.new('split', region)
	local newVolume = newLayer:volume()
	newLayer:setPalette(node:palette())

	local visitor = function (volume, x, y, z)
		newVolume:setVoxel(x, y, z, volume:voxel(x, y, z))
		volume:setVoxel(x, y, z, -1)
	end

	local palette = node:palette()
	local condition
	if exactmatch then
		condition = function (volume, x, y, z)
			local voxel = volume:voxel(x, y, z)
			if voxel == color then
				return true
			end
			return false
		end
	else
		condition = function (volume, x, y, z)
			local voxel = volume:voxel(x, y, z)
			local dist = palette:deltaE(voxel, color)
			if dist < 48 then
				return true
			end
			return false
		end
	end
	vol.conditionYXZ(node:volume(), region, visitor, condition)

	newVolume:crop()
end
