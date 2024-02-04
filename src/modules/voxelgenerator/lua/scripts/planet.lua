--
-- Build a small noise based planet in the center of the region
--

local perlin = require "modules.perlin"

function arguments()
	return {
		{ name = 'size', desc = 'size of the planet', type = 'int', default = '15', min = '10', max = '255' }
	}
end

function main(node, region, color, size)
	local volume = node:volume()
	perlin:load()
	local colorwater = color
	local land = {2, 3, 4, 5, 6}
	local freq = 1 / (size * 0.66)
	local center = region:center()
	for x = -size, size do
		--local distanceX = x ^ 2
		for y = -size, size do
			--local distanceY = y ^ 2
			for z = -size, size do
				--local distanceZ = z ^ 2
				--local distance = math.sqrt(distanceX + distanceY + distanceZ)
				--if distance < size then
					local n = perlin:norm(perlin:noise((x + 100) * freq, (y + 100) * freq, (z + 100) * freq))
					local depth = math.floor(size - math.max(math.abs(x), math.abs(y), math.abs(z)) + 0.5)
					if depth > 3 then
						volume:setVoxel(center.x + x, center.y + y, center.z + z, colorwater)
					elseif n + depth / 10 > 0.65 then
						volume:setVoxel(center.x + x, center.y + y, center.z + z, land[math.min(depth, 4) + 1])
					end
				--end
			end
		end
	end
end
