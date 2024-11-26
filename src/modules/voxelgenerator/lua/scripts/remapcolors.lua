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
	-- dark blue
	colormap[49] = 27
	colormap[50] = 27
	colormap[51] = 27
	colormap[52] = 27
	colormap[53] = 27
	colormap[48] = 27
	colormap[47] = 27
	-- light blue
	colormap[46] = 28
	colormap[45] = 28
	colormap[44] = 28
	colormap[43] = 28
	colormap[42] = 28
	colormap[41] = 28

	-- darker gray
	colormap[40] = 32
	colormap[39] = 32
	colormap[38] = 32
	colormap[37] = 32
	colormap[36] = 32
	colormap[35] = 32
	colormap[34] = 32
	colormap[33] = 32

	-- lighter gray
	colormap[32] = 33
	colormap[31] = 33
	colormap[30] = 33
	colormap[29] = 33
	colormap[28] = 33
	colormap[26] = 33
	colormap[24] = 33
	colormap[22] = 33
	colormap[20] = 33
	colormap[17] = 33
	colormap[14] = 33

	-- brown
	colormap[27] = 8
	colormap[25] = 8
	colormap[23] = 8
	colormap[21] = 8
	colormap[19] = 8
	colormap[18] = 8
	colormap[16] = 8
	colormap[15] = 8
	colormap[13] = 8
	colormap[12] = 8
	colormap[11] = 8
	colormap[9] = 8
	colormap[7] = 8

	-- yellow
	colormap[10] = 9
	colormap[8] = 9
	colormap[6] = 9
	colormap[5] = 9
	colormap[4] = 9
	colormap[3] = 9
	colormap[2] = 9

	-- white
	colormap[1] = 41

	local newpalette = g_palette.new()
	newpalette:load(palettename)

	for voxel, color in pairs(colormap) do
		local from = node:palette():colorString(voxel)
		local to = newpalette:colorString(color)
		g_log.info("change color from " .. from .. " " .. voxel .. " to color " .. to .. " " .. color)
	end

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

	node:setPalette(newpalette, false)
end
