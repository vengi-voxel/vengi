--
-- applies a gradient to the volume
--

function arguments()
	return {
		{ name = 'minheight', desc = 'the minimum height to keep at the edges', type = 'int', default = '0', min = '0', max = '255' },
	}
end

function main(node, region, _, minheight)
	local center = region:center()
	local width = region:width()
	local height = region:height()
	local depth = region:depth()
	local maxDistance = math.sqrt(center.x ^ 2 + center.y ^ 2)
	local volume = node:volume()

	for x = 0, width do
		for z = 0, depth do
			local distance = math.sqrt((x - center.x) ^ 2 + (z - center.z) ^ 2)
			local normalizedDistance = distance / maxDistance
			local gradient = 1 - normalizedDistance
			local squaredGradient = gradient ^ 2
			local maxHeight = math.floor(height * squaredGradient)
			for y = height, minheight, -1 do
				if y >= maxHeight then
					volume:setVoxel(x, y, z, -1)
				end
			end
		end
	end
end
