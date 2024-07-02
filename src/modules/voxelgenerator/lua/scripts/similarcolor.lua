--
-- distribute similar color values to the given input color
--

local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'density', desc = 'the voxel replacement density', type = 'int', default = '4', min = '1' },
		{ name = 'colors', desc = 'the color variations', type = 'int', default = '4', min = '1', max = '255' }
	}
end

function main(node, region, color, density, colors)
	local cnt = 0

	local newindices = node:palette():similar(color, colors)
	local visitor = function (volume, x, y, z)
		local indidx = math.random(1, #newindices)
		local c = newindices[indidx]
		volume:setVoxel(x, y, z, c)
	end

	local nextCnt = math.random(1, density)
	local condition = function (volume, x, y, z)
		local voxel = volume:voxel(x, y, z)
		if voxel ~= -1 then
			if voxel == color then
				cnt = cnt + 1
				if cnt % nextCnt == 0 then
					cnt = 0
					nextCnt = math.random(1, density)
					return true
				end
				return false;
			end
		end
		return false
	end
	vol.conditionYXZ(node:volume(), region, visitor, condition)
end
