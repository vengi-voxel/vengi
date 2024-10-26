--
-- Generate Mandelbulb fractal
--

local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'power', desc = 'The power for the Mandelbulb fractal formula.', type = 'float', default = '8', min = '1', max = '12' },
		{ name = 'iterations', type = 'int', default = '10', min = '1', max = '20' },
		{ name = 'threshold', desc = 'Threshold for escape condition.', type = 'float', default = '2.0', min = '0.5', max = '4.0' }
	}
end

function main(node, region, color, power, iterations, threshold)
	local visitor = function(volume, x, y, z)
		volume:setVoxel(x, y, z, color)
	end

	local condition = function(volume, x, y, z)
		local width = region:width()
		local height = region:height()
		local depth = region:depth()
		local nx = (x / width - 0.5) * 2
		local ny = (y / height - 0.5) * 2
		local nz = (z / depth - 0.5) * 2

		-- Initialize Mandelbulb parameters
		local zx, zy, zz = nx, ny, nz
		local dr = 1.0

		for _ = 1, iterations do
			local r = math.sqrt(zx * zx + zy * zy + zz * zz)
			if r > threshold then
				return false -- Point escapes, do not set voxel
			end

			-- Convert to polar coordinates
			local theta = math.acos(zz / r)
			local phi = math.atan(zy, zx)
			dr = dr * power * r ^ (power - 1.0)

			-- Mandelbulb formula
			local zr = r ^ power
			theta = theta * power
			phi = phi * power

			zx = zr * math.sin(theta) * math.cos(phi) + nx
			zy = zr * math.sin(theta) * math.sin(phi) + ny
			zz = zr * math.cos(theta) + nz
		end
		return true
	end

	vol.conditionYXZ(node:volume(), region, visitor, condition)
end
