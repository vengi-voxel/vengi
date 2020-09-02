local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'amount', desc = 'the amount of voxel to add', type = 'int', default = '1' },
		{ name = 'thickencolor', desc = 'a value of -1 means everything except air - otherwise it is taken as palette index', type = 'int', default = '-1' }
	}
end

function main(volume, region, color, amount, thickencolor)
	local activeLayer = layermgr.get()
	local newName = activeLayer:name() .. "_thickened"
	local newLayer = layermgr.new(newName, true, region)
	local newVolume = newLayer:volume()

	local visitor = function (volume, x, y, z)
		local voxel = volume:voxel(x, y, z)
		for cubeX = x, x + amount do
			for cubeZ = z, z + amount do
				for cubeY = y, y + amount do
					newVolume:setVoxel(cubeX, cubeY, cubeZ, voxel)
				end
			end
		end
	end

	local condition = function (volume, x, y, z)
		local voxel = volume:voxel(x, y, z)
		if thickencolor == -1 then
			if voxel ~= -1 then
				return true
			end
		else
			if voxel == thickencolor then
				return true
			end
		end
		return false
	end
	vol.conditionXYZ(volume, region, visitor, condition)
end
