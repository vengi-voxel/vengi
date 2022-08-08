--
-- build a pyramid in the given range
--

function arguments()
	return {
		{ name = 'n', desc = 'height level delta', type = 'int', default = '1' }
	}
end

function main(node, region, color, n)
	local volume = node:volume()
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
