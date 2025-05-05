--
-- This will compact given region of a node by a percentage along a specified axis.
--

local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'percent', desc = 'percentage', type = 'int', default = '50', min = '0', max = '100' },
		{ name = 'axis', desc = 'axis', type = 'enum', enum = 'x,y,z', default = 'y' }
	}
end

function main(node, region, _, percent, axis)
	local newName = node:name() .. "_compaction"
	local newLayer = g_scenegraph.new(newName, region)
	newLayer:setPalette(node:palette())
	local newVolume = newLayer:volume()
	local mins = region:mins()
	local visitor = function (volume, x, y, z)
		local voxel = volume:voxel(x, y, z)
		if voxel == -1 then
			return
		end
		local nx = x - mins.x
		local ny = y - mins.y
		local nz = z - mins.z
		if axis == "x" then
			nx = math.floor(nx * percent / 100)
		elseif axis == "y" then
			ny = math.floor(ny * percent / 100)
		elseif axis == "z" then
			nz = math.floor(nz * percent / 100)
		else
			error("Invalid axis: " .. axis)
		end
		newVolume:setVoxel(mins.x + nx, mins.y + ny, mins.z + nz, voxel)
	end
	vol.visitYXZ(node:volume(), region, visitor)
	node:hide()
end
