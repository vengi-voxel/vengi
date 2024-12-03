local util = require('modules.util')

function arguments()
	return {
		{name = "nametexture", desc = "the image name for the color texture", type = "file"},
		{name = "namedepthmap", desc = "the image name for the depthmap", type = "file", default = ""},
		{name = "thickness", desc = "the strength factor of the depth map values", type = "int", default = "8", min = "1"},
		{name = "bothsides", desc = "the y size of the grid", type = "bool", default = "true"}
	}
end

function main(node, _, _, nametexture, namedepthmap, thickness, bothsides)
	if namedepthmap == "" then
		local ext = util.getFileExtension(nametexture)
		namedepthmap = nametexture:sub(1, #nametexture - #ext) .. "-dm" .. ext
	end
	node:volume():importImageAsVolume(nametexture, namedepthmap, node:palette(), thickness, bothsides)
end
