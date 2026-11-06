local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'newcolor', desc = 'the palette color index', type = 'colorindex' }
	}
end

function description()
	return "Replaces one palette color with another one."
end

function main(node, region, color, newcolor)
	vol.replaceColor(node, region, color, newcolor)
end
