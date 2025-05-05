local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'empty', desc = 'The amount of empty voxels that will lead to a removal.', type = 'int', default = '24', min = '1', max = '26' },
		{ name = 'nonempty', desc = 'The amount of non-empty voxels that will lead to a new voxel.', type = 'int', default = '16', min = '1', max = '25' }
	}
end

function description()
	return "Fills minor gaps between voxels or removes voxels that are not well connected."
end

function main(node, region, color, empty, nonempty)
	local visitor = function (volume, x, y, z)
		local emptyVoxels = vol.countEmptyAround(volume, x, y, z, 1)
		local mins = region:mins()
		local maxs = region:maxs()
		local edges = 0
		if (mins.x == x or maxs.x == x) then
			edges = edges + 1
		end
		if (mins.y == y or maxs.y == y) then
			edges = edges + 1
		end
		if (mins.z == z or maxs.z == z) then
			edges = edges + 1
		end
		if (edges == 3) then
			emptyVoxels = emptyVoxels - 19
		end
		if (edges == 2) then
			emptyVoxels = emptyVoxels - 15
		end
		if (edges == 1) then
			emptyVoxels = emptyVoxels - 9
		end
		local voxel = volume:voxel(x, y, z)
		if (emptyVoxels >= empty) then
			if (voxel == -1) then
				return
			end
			g_log.debug("remove voxel at " .. x .. ", " .. y .. ", " .. z .. " because of " .. emptyVoxels .. " empty voxels threshold of: " .. empty)
			volume:setVoxel(x, y, z, -1)
		else
			local occupiedVoxels = 26 - emptyVoxels
			if (occupiedVoxels >= nonempty) then
				if (voxel ~= -1) then
					return
				end
				-- TODO: find the best color
				g_log.debug("add voxel at " .. x .. ", " .. y .. ", " .. z .. " because of " .. occupiedVoxels .. " occupied voxels threshold of: " .. nonempty)
				volume:setVoxel(x, y, z, color)
			end
		end
	end
	vol.visitYXZ(node:volume(), region, visitor)
end
