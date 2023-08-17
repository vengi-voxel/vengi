--
-- replace one palette with another one
--

function arguments()
	return {
		{ name = 'palettename', desc = 'the palette name', type = 'string' },
		{ name = 'remap', desc = 'false to only replace, true to remap the colors', type = 'bool', default = 'false' }
	}
end

function main(node, region, color, palettename, remap)
	local newpalette = g_palette.new()
	newpalette:load(palettename)
	node:setPalette(newpalette, remap)
end
