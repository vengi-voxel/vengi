function arguments()
	return {
		{
			name = "deviation",
			desc = "Max height deviation from slope plane",
			type = "int",
			default = "3",
			min = "0",
			max = "20"
		},
		{name = "sampledist", desc = "Sample distance for plane fitting", type = "int", default = "6", min = "1", max = "30"}
	}
end

function description()
	return "Select surface voxels along a slope plane from the cursor"
end

function icon()
	return "mountain"
end

function select(node, region, deviation, sampledist)
	local volume = node:volume()
	local cursor = g_selectioncontext.cursorPos()
	local face = g_selectioncontext.cursorFace()
	local modType = g_selectioncontext.modifierType()
	local selecting = modType ~= "erase"

	if face == "max" then
		return
	end

	if volume:voxel(cursor.x, cursor.y, cursor.z) == -1 then
		return
	end

	volume:visitSlopeSurface(
		cursor.x,
		cursor.y,
		cursor.z,
		face,
		deviation,
		sampledist,
		function(x, y, z)
			volume:setSelected(x, y, z, selecting)
		end
	)
end
