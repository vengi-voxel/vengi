--
-- replace the colors in a model and set a new palette. The colors in the
-- colormap are just examples. You can replace them with the colors you want.
--

local vol = require "modules.volume"

function arguments()
	return {
		{ name = 'palettename', desc = 'the palette name', type = 'string', default = 'built-in:nippon' },
	}
end

function main(node, region, _, palettename)
	-- build a map of color indices that we should convert
	-- from = to
	local colormap = {}
	colormap[49] = 27
	colormap[50] = 28
	colormap[51] = 29

	local visitor = function (volume, x, y, z)
		local voxel = volume:voxel(x, y, z)
		local color = colormap[voxel]
		volume:setVoxel(x, y, z, color)
	end

	-- only visit those voxels that you've defined in the colormap above
	local condition = function (volume, x, y, z)
		local voxel = volume:voxel(x, y, z)
		local color = colormap[voxel]
		return color ~= nil
	end
	vol.conditionYXZ(node:volume(), region, visitor, condition)

	local newpalette = g_palette.new()
	newpalette:load(palettename)
	node:setPalette(newpalette, false)
end
