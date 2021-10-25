local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'freq', desc = 'frequence for the noise function input', type = 'float', default = '0.05' },
		{ name = 'amplitude', desc = 'amplitude to scale the noise noise function output', type = 'float', default = '0.3' },
		{ name = 'dimensions', desc = 'the dimensions of the noise call', type = 'int', min = '2', max = '3', default = '2' },
		{ name = 'threshold', desc = 'the 3d noise threshold to place a voxel', type = 'float', min = '0.0', max = '1.0', default = '0.15' },
		{ name = 'type', type = 'enum', enum = 'simplex,worley', default = 'simplex'}
	}
end

local function noise2d(volume, region, color, freq, amplitude, type)
	local visitorSimplex = function (volume, x, z)
		local maxY = amplitude * noise.noise2(vec2.new(x * freq, z * freq)) * region:height()
		for y = 0, maxY do
			volume:setVoxel(x, y, z, color)
		end
	end
	local visitorWorley = function (volume, x, z)
		local maxY = amplitude * noise.worley2(vec2.new(x * freq, z * freq)) * region:height()
		for y = 0, maxY do
			volume:setVoxel(x, y, z, color)
		end
	end

	local visitor = visitorSimplex
	if (type == 'worley') then
		visitor = visitorWorley
	end
	vol.visitXZ(volume, region, visitor)
end

local function noise3d(volume, region, color, freq, amplitude, threshold, type)
	local visitorSimplex = function (volume, x, y, z)
		local v = vec3.new(x * freq, y * freq, z * freq);
		local val = amplitude * noise.noise3(v)
		if (val > threshold) then
			volume:setVoxel(x, y, z, color)
		end
	end

	local visitorWorley = function (volume, x, y, z)
		local v = vec3.new(x * freq, y * freq, z * freq);
		local val = amplitude * noise.worley3(v)
		if (val > threshold) then
			volume:setVoxel(x, y, z, color)
		end
	end
	local visitor = visitorSimplex
	if (type == 'worley') then
		visitor = visitorWorley
	end
	vol.visitYXZ(volume, region, visitor)
end

function main(volume, region, color, freq, amplitude, dimensions, threshold, type)
	if (dimensions == 2) then
		noise2d(volume, region, color, freq, amplitude, type)
	else
		noise3d(volume, region, color, freq, amplitude, threshold, type)
	end
end
