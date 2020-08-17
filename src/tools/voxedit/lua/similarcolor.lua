local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'density', desc = 'the voxel replacement density', type = 'int' },
		{ name = 'colors', desc = 'the color variations', type = 'int' }
	}
end

function main(volume, region, color, density, colors)
	local cnt = 0

	-- TODO: find similar colors and put them into the newindices
	local newindices = {5, 6, 7, 8, 9}
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
				return cnt % density == 0;
			end
		end
		return false;
	end
	vol.conditionXYZ(volume, region, visitor, condition)
end
