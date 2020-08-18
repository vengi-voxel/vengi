function arguments()
	return {
		{ name = 'n', desc = 'height level', type = 'int', default = '1' }
	}
end

function main(volume, region, color, n)
	local mins = region:mins()
	local maxs = region:maxs()
	for x = mins.x, maxs.x do
		for z = mins.z, maxs.z do
			for y = maxs.y, mins.y, -1 do
				if volume:voxel(x, y, z) ~= -1 then
					for h = 1, n do
						volume:setVoxel(x, y + h, z, color)
					end
					break
				end
			end
		end
	end
end
