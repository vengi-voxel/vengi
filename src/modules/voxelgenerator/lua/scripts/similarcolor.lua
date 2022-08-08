--
-- distribute similar color values to the given input color
--

local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'density', desc = 'the voxel replacement density', type = 'int', default = '4' },
		{ name = 'colors', desc = 'the color variations', type = 'int', default = '4' }
	}
end

function main(node, region, color, density, colors)
	local volume = node:volume()
	local cnt = 0

	local newindices = node:palette().similar(color, colors)
	local visitor = function (volume, x, y, z)
		local indidx = math.random(1, #newindices)
		local c = newindices[indidx]
		volume:setVoxel(x, y, z, c)
	end

	local condition = function (volume, x, y, z)
		local voxel = volume:voxel(x, y, z)
		if voxel ~= -1 then
			if voxel == color then
				cnt = cnt + 1
				return cnt % density == 0
			end
		end
		return false
	end
	vol.conditionYXZ(volume, region, visitor, condition)
end
