--
-- place n voxels on top of the surface - allows e.g. to put snow on everything.
--
local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'n', desc = 'height level', type = 'int', default = '1' }
	}
end

function main(node, region, color, n)
	local mins = region:mins()
	local maxs = region:maxs()
	local visitor = function (volume, x, z)
		for y = maxs.y, mins.y, -1 do
			if volume:voxel(x, y, z) ~= -1 then
				for h = 1, n do
					volume:setVoxel(x, y + h, z, color)
				end
				break
			end
		end
	end
	vol.visitXZ(node:volume(), region, visitor)
end
