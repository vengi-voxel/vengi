--
-- moves all connected voxels of the node's volume into new volumes
--
-- split all single identifiable objects into own models/nodes
--

local vol = require "modules.volume"

local splitVolume = {}
local needNewModel = false
local palette = {}

local function connectedVisitor(volume, x, y, z)
	if needNewModel then
		local newModel = g_scenegraph.new('splitobject', volume:region())
		splitVolume = newModel:volume()
		newModel:setPalette(palette)
		needNewModel = false
	end
	local color = volume:voxel(x, y, z)
	splitVolume:setVoxel(x, y, z, color)
	volume:setVoxel(x, y, z, -1)
end

function main(node, region, _)
	palette = node:palette()
	local visitor = function (loopVolume, x, y, z)
		if loopVolume:voxel(x, y, z) ~= -1 then
			needNewModel = true
			splitVolume = nil
			vol.visitConnected6(loopVolume, x, y, z, connectedVisitor)
			if splitVolume ~= nil then
				splitVolume:crop()
			end
		end
	end
	vol.visitYXZ(node:volume(), region, visitor)
end
