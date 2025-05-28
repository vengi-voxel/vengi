--
-- genland algorithm by Tom Dobrowolski
--

function arguments()
	return {
		{ name = 'seed', type = 'int', default = 0, min = 0, max = 1000000, desc = 'Seed for the random number generator.' },
		{ name = 'size', type = 'enum', default = '256', enum = '32,64,128,256,512,1024', desc = 'Size of the generated landscape. Must be a power of two.' },
		{ name = 'height', type = 'int', default = 64, min = 32, max = 256, desc = 'Height of the generated landscape.' },
		{ name = 'octaves', type = 'int', default = 10, min = 1, max = 30, desc = 'Number of octaves for the noise generation.' },
		{ name = 'smoothing', type = 'int', default = 1, min = 1, max = 3, desc = 'Apply smoothing to the generated landscape.' },
		{ name = 'persistence', type = 'float', default = 0.4, min = 0.0, max = 1.0, desc = 'Persistence for the noise generation - applied to the amplitude.' },
		{ name = 'amplitude', type = 'float', default = 1.0, min = 0.01, max = 5.0, desc = 'Amplitude for the noise generation - applied to the height.' },
		{ name = 'riverWidth', type = 'float', default = 0.02, min = 0.01, max = 0.1, desc = 'Width of the rivers in the landscape.' },
		{ name = 'freqGround', type = 'float', default = 9.5, min = 0.1, max = 20.0, desc = 'Frequency of the ground noise.' },
		{ name = 'freqRiver', type = 'float', default = 13.2, min = 0.1, max = 20.0, desc = 'Frequency of the river noise.' },
		{ name = 'river', type = 'bool', default = 'true', desc = 'Generate rivers in the landscape.' },
		{ name = 'shadow', type = 'bool', default = 'true', desc = 'Generate shadows in the landscape.' }
	}
end

function description()
	return "Genland - procedural landscape generator."
end

function main(_, _, _, seed, size, height, octaves, smoothing, persistence, amplitude, riverWidth, freqGround, freqRiver, river, shadow)
	-- TODO: colors
	g_algorithm.genland(seed, tonumber(size), height, octaves, smoothing, persistence, amplitude, riverWidth, freqGround, freqRiver, river, shadow)
end
