--
-- separates all models in a scene by packing them tightly together at the ground
--

function arguments()
	return {
		{ name = 'padding', desc = 'padding between nodes', type = 'int', default = '2' }
	}
end

function main(node, region, color, padding)
	g_scenegraph.align(padding)
end
