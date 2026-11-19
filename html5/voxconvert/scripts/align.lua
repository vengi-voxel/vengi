function arguments()
	return {
		{ name = 'padding', desc = 'padding between nodes', type = 'int', default = '2' }
	}
end

function description()
	return "Aligns the nodes in the scene graph to the ground with a specified padding."
end

function main(node, region, color, padding)
	g_scenegraph.align(padding)
end
