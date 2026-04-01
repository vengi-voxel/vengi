function arguments()
	return {
		{ name = 'connectivity', desc = 'Connectivity type', type = 'enum', default = '18', enum = '6,18,26' }
	}
end

function description()
	return "Draws a path over existing voxels from the reference position to the cursor"
end

function icon()
	return "footprints"
end

function generate(node, region, color, connectivity)
	local volume = node:volume()
	local ref = g_brushcontext.referencePos()
	local cursor = g_brushcontext.cursorPos()
	local path = volume:pathfinder(ref.x, ref.y, ref.z, cursor.x, cursor.y, cursor.z, connectivity)
	if path == nil then
		return
	end
	for _, pos in ipairs(path) do
		volume:setVoxel(pos.x, pos.y, pos.z, color)
	end
end
