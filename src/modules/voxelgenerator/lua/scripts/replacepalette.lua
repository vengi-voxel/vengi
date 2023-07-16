--
-- replace one palette with another one
--

function arguments()
	return {
		{ name = 'palettename', desc = 'the palette name', type = 'string' },
		{ name = 'remap', desc = 'false to only replace, true to remap the colors', type = 'bool' }
	}
end

function main(node, region, color, palettename, remap)
	local newpalette = palettemgr.new()
	newpalette:load(palettename)
	node:setPalette(newpalette, remap)
end
