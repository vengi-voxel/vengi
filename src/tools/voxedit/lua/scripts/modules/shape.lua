local module = {}

function module.createCube(volume, minsx, minsy, minsz, maxsx, maxsy, maxsz, voxel)
	for x = minsx, maxsx do
		for z = minsz, maxsz do
			for y = minsy, maxsy do
				volume:setVoxel(x, y, z, voxel)
			end
		end
	end
end

return module
