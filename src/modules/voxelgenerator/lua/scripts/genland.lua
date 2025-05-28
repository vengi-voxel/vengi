--
-- genland algorithm by Tom Dobrowolski
--

function arguments()
	return {
		{ name = 'seed', type = 'int', default = 0, min = 0, max = 1000000, description = 'Seed for the random number generator.' },
		{ name = 'height', type = 'int', default = 64, min = 32, max = 256, description = 'Height of the generated landscape.' },
		{ name = 'ovtaves', type = 'int', default = 10, min = 1, max = 30, description = 'Number of octaves for the noise generation.' }
	}
end

function description()
	return "Genland - procedural landscape generator."
end

function main(_, _, _, seed, height, ovtaves)
	g_algorithm.genland(seed, height, ovtaves)
end
