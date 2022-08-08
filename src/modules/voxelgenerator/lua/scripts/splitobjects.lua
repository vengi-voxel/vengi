--
-- moves all connected voxels of the node's volume into new volumes
--

local vol = require "modules.volume"

local splitVolume = {}
local needNewLayer = false

local function connectedVisitor(volume, x, y, z)
	if needNewLayer then
		local newLayer = scenegraph.new('splitobject', volume:region())
		splitVolume = newLayer:volume()
		needNewLayer = false
	end
	local color = volume:voxel(x, y, z)
	splitVolume:setVoxel(x, y, z, color)
	volume:setVoxel(x, y, z, -1)
end

function main(node, region, color)
	vol.visitYXZ(node:volume(), region, function (loopVolume, x, y, z)
		if loopVolume:voxel(x, y, z) ~= -1 then
			needNewLayer = true
			vol.visitConnected6(loopVolume, x, y, z, connectedVisitor)
		end
	end)
end
