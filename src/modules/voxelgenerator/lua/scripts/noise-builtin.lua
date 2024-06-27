local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'freq', desc = 'frequence for the noise function input', type = 'float', default = '0.05' },
		{ name = 'amplitude', desc = 'amplitude to scale the noise noise function output', type = 'float', default = '0.3' },
		{ name = 'dimensions', desc = 'the dimensions of the noise call', type = 'int', min = '2', max = '3', default = '2' },
		{ name = 'threshold', desc = 'the 3d noise threshold to place a voxel', type = 'float', min = '0.0', max = '1.0', default = '0.15' },
		{ name = 'type', type = 'enum', enum = 'simplex,worley', default = 'simplex'},
		{ name = 'seed', desc = 'seed for the noise function', type = 'float', default = '0.0'}
	}
end

local function noise2d(volume, region, color, freq, amplitude, type, seed)
	local visitor = function (noiseVolume, x, z)
		if noiseVolume == nil then
			error("volume is nil")
		end
		local maxY = amplitude * g_noise.noise2(seed + x * freq, seed + z * freq) * region:height()
		for y = 0, maxY do
			noiseVolume:setVoxel(x, y, z, color)
		end
	end
	if (type == 'worley') then
		visitor = function (noiseVolume, x, z)
			if noiseVolume == nil then
				error("volume is nil")
			end
			local maxY = amplitude * g_noise.worley2(seed + x * freq, seed + z * freq) * region:height()
			for y = 0, maxY do
				noiseVolume:setVoxel(x, y, z, color)
			end
		end
	end
	vol.visitXZ(volume, region, visitor)
end

local function noise3d(volume, region, color, freq, amplitude, threshold, type, seed)
	local visitorSimplex = function (noiseVolume, x, y, z)
		if noiseVolume == nil then
			error("volume is nil")
		end
		local val = amplitude * g_noise.noise3(seed + x * freq, seed + y * freq, seed + z * freq)
		if (val > threshold) then
			noiseVolume:setVoxel(x, y, z, color)
		end
	end

	local visitorWorley = function (noiseVolume, x, y, z)
		if noiseVolume == nil then
			error("volume is nil")
		end
		local val = amplitude * g_noise.worley3(seed + x * freq, seed + y * freq, seed + z * freq)
		if (val > threshold) then
			noiseVolume:setVoxel(x, y, z, color)
		end
	end
	local visitor = visitorSimplex
	if (type == 'worley') then
		visitor = visitorWorley
	end
	vol.visitYXZ(volume, region, visitor)
end

function main(node, region, color, freq, amplitude, dimensions, threshold, type, seed)
	local volume = node:volume()
	if (dimensions == 2) then
		noise2d(volume, region, color, freq, amplitude, type, seed)
	else
		noise3d(volume, region, color, freq, amplitude, threshold, type, seed)
	end
end
