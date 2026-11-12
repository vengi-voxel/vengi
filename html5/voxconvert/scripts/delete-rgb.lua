local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'r', desc = 'red [0-255]', type = 'int', default = '0', min = '0', max = '255' },
		{ name = 'g', desc = 'green [0-255]', type = 'int', default = '0', min = '0', max = '255' },
		{ name = 'b', desc = 'blue [0-255]', type = 'int', default = '0', min = '0', max = '255' }
	}
end

function description()
	return "Deletes all voxels of a given color in the scene graph. It takes the closest palette index."
end

function main(node, region, _, r, g, b)
	local colormatch = node:palette():match(r, g, b)
	vol.replaceColor(node, region, colormatch, -1)
end
