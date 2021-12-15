function arguments()
	return {
		{ name = 'n', desc = 'height level delta', type = 'int', default = '1' }
	}
end

function main(volume, region, color, n)
	local mins = region:mins()
	local maxs = region:maxs()
	local subtract = 0
	for y = mins.y, maxs.y do
		for x = mins.x + subtract, maxs.x - subtract do
			for z = mins.z + subtract, maxs.z - subtract do
				volume:setVoxel(x, y, z, color)
			end
		end
		if y % n == 0 then
			subtract = subtract + 1
		end
	end
end
