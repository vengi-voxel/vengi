--
-- Shadow calculation for a volume
--

function arguments()
	return {
		{ name = 'lightstep', desc = 'The color steps for a shadow value', type = 'int', default = '8' },
	}
end

function description()
	return "Apply shadows to a volume"
end

function main(node, _, _, lightstep)
	g_algorithm.shadow(node:volume(), lightstep)
end
