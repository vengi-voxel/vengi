--
-- genland algorithm by Tom Dobrowolski
--

function arguments()
	return {
		{ name = 'seed', type = 'int', default = 0, min = 0, max = 1000000, desc = 'Seed for the random number generator.' },
		{ name = 'size', type = 'enum', default = '256', enum = '32,64,128,256,512,1024', desc = 'Size of the generated landscape. Must be a power of two.' },
		{ name = 'height', type = 'int', default = 64, min = 32, max = 1024, desc = 'Volume height; columns are clipped to this.' },
		{ name = 'octaves', type = 'int', default = 10, min = 1, max = 30, desc = 'Number of octaves for the noise generation.' },
		{ name = 'smoothing', type = 'int', default = 1, min = 1, max = 3, desc = 'Apply smoothing to the generated landscape shadows.' },
		{ name = 'persistence', type = 'float', default = 0.4, min = 0.0, max = 1.0, desc = 'Octave amplitude falloff.' },
		{ name = 'amplitude', type = 'float', default = 20.0, min = 0.0, max = 1000.0, desc = 'How strongly noise pushes terrain up and down from base height.' },
		{ name = 'baseHeight', type = 'float', default = 28.0, min = 0.0, max = 1000.0, desc = 'Average column height before noise is applied.' },
		{ name = 'riverWidth', type = 'float', default = 0.02, min = 0.0, max = 1.0, desc = 'Width of the rivers in the landscape.' },
		{ name = 'numRivers', type = 'int', default = 1, min = 0, max = 100, desc = 'How many rivers to attempt across the map.' },
		{ name = 'riverPhase', type = 'float', default = 0.75, min = 0.0, max = 1.0, desc = 'Where rivers begin horizontally (0 = far left, 1 = far right).' },
		{ name = 'riverMeander', type = 'float', default = 4.0, min = 0.0, max = 20.0, desc = 'How strongly river noise warps the path (0 = straight).' },
		{ name = 'freqGround', type = 'float', default = 9.5, min = 0.1, max = 100.0, desc = 'Frequency of the ground noise.' },
		{ name = 'freqRiver', type = 'float', default = 13.2, min = 0.1, max = 100.0, desc = 'Frequency of the river noise.' },
		{ name = 'grassBias', type = 'float', default = 0.0, min = -1.0, max = 1.0, desc = 'Shifts grass tint (positive = more grass, negative = more ground).' },
		{ name = 'offsetx', type = 'int', default = 0, min = 0, max = 100000, desc = 'X offset for the landscape generation.' },
		{ name = 'offsety', type = 'int', default = 0, min = 0, max = 100000, desc = 'Y offset for the landscape generation.' },
		{ name = 'shadow', type = 'bool', default = 'true', desc = 'Generate shadows in the landscape.' },
		{ name = 'shadowFactor', type = 'int', default = 32, min = 0, max = 255, desc = 'Shadow strength from the simulated sun.' },
		{ name = 'river', type = 'bool', default = 'true', desc = 'Generate rivers in the landscape.' },
		{ name = 'ambience', type = 'bool', default = 'true', desc = 'Apply ambient lighting to the landscape.' },
		{ name = 'ambienceFactor', type = 'float', default = 0.3, min = 0.0, max = 1.0, desc = 'Ambient color scale.' },
		{ name = 'groundColor', type = 'hexcolor', default = '#8C7D73', desc = 'Ground color.' },
		{ name = 'grassColor', type = 'hexcolor', default = '#485020', desc = 'Primary grass color.' },
		{ name = 'grass2Color', type = 'hexcolor', default = '#444E28', desc = 'Secondary grass color.' },
		{ name = 'waterColor', type = 'hexcolor', default = '#3C6478', desc = 'Water color.' }
	}
end

function description()
	return "Genland - procedural landscape generator by Tom Dobrowolski."
end

function main(_, _, _, seed, size, height, octaves, smoothing, persistence, amplitude, baseHeight, riverWidth, numRivers, riverPhase, riverMeander, freqGround, freqRiver, grassBias, offsetx, offsety, shadow, shadowFactor, river, ambience, ambienceFactor, groundColor, grassColor, grass2Color, waterColor)
	g_algorithm.genland(seed, tonumber(size), height, octaves, smoothing, persistence, amplitude, baseHeight, riverWidth, numRivers, riverPhase, riverMeander, freqGround, freqRiver, grassBias, offsetx, offsety, shadow, shadowFactor, river, ambience, ambienceFactor, groundColor, grassColor, grass2Color, waterColor)
end
