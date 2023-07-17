--
-- fill hollows in a model
--

local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'fillvoxel', desc = 'the palette index of the color to use', type = 'colorindex' },
	}
end

function main(node, region, color, fillvoxel)
	local volume = node:volume()
	volume:fillHollow(fillvoxel)
end
