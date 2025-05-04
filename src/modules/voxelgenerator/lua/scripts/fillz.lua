--
-- fill from the bottom to the top until a voxel is found
--

function main(node, region, color)
	local volume = node:volume()
	local mins = region:mins()
	local maxs = region:maxs()
	for x = mins.x, maxs.x do
		coroutine.yield()
		for z = mins.z, maxs.z do
			local maxY = -1
			for y = mins.y, maxs.y do
				local voxel = volume:voxel(x, y, z)
				if voxel ~= -1 then
					maxY = y - 1
					break
				end
			end
			if maxY ~= -1 then
				for y = mins.y, maxY do
					volume:setVoxel(x, y, z, color)
				end
			end
		end
	end
end
