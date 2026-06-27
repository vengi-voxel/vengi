function arguments()
	return {
		{name = "width", desc = "Line thickness in voxels", type = "int", default = "1", min = "1", max = "32"},
		{
			name = "pathMode",
			desc = "Follow the visible surface or draw a straight 3D chord",
			type = "enum",
			default = "surface",
			enum = "surface,straight"
		}
	}
end

function description()
	return "Select voxels along a line between the two drag corners"
end

function icon()
	return "penline"
end

function gizmo()
	local startPos = g_selectioncontext.aabbFirstPos()
	local endPos = g_selectioncontext.cursorPos()
	local face = g_selectioncontext.aabbFace()
	if face == "max" then
		return nil
	end
	if startPos.x == endPos.x and startPos.y == endPos.y and startPos.z == endPos.z then
		return nil
	end
	return {
		operations = {"line"},
		positions = {
			{startPos.x + 0.5, startPos.y + 0.5, startPos.z + 0.5},
			{endPos.x + 0.5, endPos.y + 0.5, endPos.z + 0.5}
		}
	}
end

function select(node, region, width, pathMode)
	local volume = node:volume()
	local startPos = g_selectioncontext.aabbFirstPos()
	local endPos = g_selectioncontext.cursorPos()
	local face = g_selectioncontext.aabbFace()
	local modType = g_selectioncontext.modifierType()
	local selecting = modType ~= "erase"

	if face == "max" then
		return
	end

	local function mark(x, y, z)
		volume:setSelected(x, y, z, selecting)
	end

	if pathMode == "surface" then
		volume:lineDrapeSurface(
			startPos.x,
			startPos.y,
			startPos.z,
			endPos.x,
			endPos.y,
			endPos.z,
			face,
			width,
			mark
		)
	else
		volume:lineMarkSolid(
			startPos.x,
			startPos.y,
			startPos.z,
			endPos.x,
			endPos.y,
			endPos.z,
			width,
			mark
		)
	end
end
