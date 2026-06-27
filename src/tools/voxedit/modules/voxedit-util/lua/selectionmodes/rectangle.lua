function arguments()
	return {
		{name = "edgeWidth", desc = "Thickness of the rectangle outline in voxels", type = "int", default = "1", min = "1", max = "32"},
		{name = "angle", desc = "Rotation of the rectangle in the surface plane (degrees)", type = "int", default = "0", min = "-180", max = "180"},
		{
			name = "surfaceTolerance",
			desc = "How far corners may deviate from the reference depth to snap to the surface",
			type = "int",
			default = "64",
			min = "0",
			max = "64"
		}
	}
end

function description()
	return "Select a rotated rectangle outline draped over the visible surface"
end

function icon()
	return "squaredashed"
end

local sel = require("modules.selection")

local function round(n)
	if n >= 0 then
		return math.floor(n + 0.5)
	end
	return math.ceil(n - 0.5)
end

local function faceAxes(face)
	local wAxis = sel.faceToAxisIndex(face)
	local uAxis = (wAxis + 1) % 3
	local vAxis = (wAxis + 2) % 3
	return uAxis, vAxis, wAxis, sel.isPositiveFace(face)
end

local function dragRegion()
	local a = g_selectioncontext.aabbFirstPos()
	local b = g_selectioncontext.cursorPos()
	return {
		mins = function()
			return {
				x = math.min(a.x, b.x),
				y = math.min(a.y, b.y),
				z = math.min(a.z, b.z)
			}
		end,
		maxs = function()
			return {
				x = math.max(a.x, b.x),
				y = math.max(a.y, b.y),
				z = math.max(a.z, b.z)
			}
		end
	}
end

local function isZeroDrag()
	local a = g_selectioncontext.aabbFirstPos()
	local b = g_selectioncontext.cursorPos()
	return a.x == b.x and a.y == b.y and a.z == b.z
end

local function isZeroAreaOnFace(box, face)
	local uAxis, vAxis = faceAxes(face)
	local rmin = box:mins()
	local rmax = box:maxs()
	local du = sel.getAxis(rmax, uAxis) - sel.getAxis(rmin, uAxis)
	local dv = sel.getAxis(rmax, vAxis) - sel.getAxis(rmin, vAxis)
	return du == 0 and dv == 0
end

local function computeCorners(box, face, angle)
	local uAxis, vAxis, wAxis, positiveNormal = faceAxes(face)
	local rmin = box:mins()
	local rmax = box:maxs()
	local cu = (sel.getAxis(rmin, uAxis) + sel.getAxis(rmax, uAxis)) * 0.5
	local cv = (sel.getAxis(rmin, vAxis) + sel.getAxis(rmax, vAxis)) * 0.5
	local hu = (sel.getAxis(rmax, uAxis) - sel.getAxis(rmin, uAxis)) * 0.5
	local hv = (sel.getAxis(rmax, vAxis) - sel.getAxis(rmin, vAxis)) * 0.5
	local w0 = positiveNormal and sel.getAxis(rmax, wAxis) or sel.getAxis(rmin, wAxis)
	local rad = math.rad(angle)
	local cs = math.cos(rad)
	local sn = math.sin(rad)
	local localCorners = {{hu, hv}, {-hu, hv}, {-hu, -hv}, {hu, -hv}}
	local corners = {}
	for i = 1, 4 do
		local lu = localCorners[i][1]
		local lv = localCorners[i][2]
		local p = {x = 0, y = 0, z = 0}
		sel.setAxis(p, uAxis, round(cu + lu * cs - lv * sn))
		sel.setAxis(p, vAxis, round(cv + lu * sn + lv * cs))
		sel.setAxis(p, wAxis, w0)
		corners[i] = p
	end
	return corners, uAxis, vAxis, wAxis, w0
end

local function snapCornersToSurface(volume, corners, uAxis, vAxis, wAxis, w0, columnTol, face)
	for i = 1, 4 do
		local c = corners[i]
		local sw = volume:findSurfaceNear(sel.getAxis(c, uAxis), sel.getAxis(c, vAxis), w0, columnTol, face)
		if sw ~= nil then
			sel.setAxis(c, wAxis, sw)
		end
	end
end

local function outlineRectangle(volume, corners, face, edgeWidth, selecting)
	local function mark(x, y, z)
		volume:setSelected(x, y, z, selecting)
	end
	for i = 1, 4 do
		local a = corners[i]
		local b = corners[(i % 4) + 1]
		volume:lineDrapeSurface(a.x, a.y, a.z, b.x, b.y, b.z, face, edgeWidth, mark)
	end
end

local function rectangleCorners(edgeWidth, angle, surfaceTolerance, volume)
	local face = g_selectioncontext.aabbFace()
	if face == "max" or isZeroDrag() then
		return nil
	end
	local box = dragRegion()
	if isZeroAreaOnFace(box, face) then
		return nil
	end
	local corners, uAxis, vAxis, wAxis, w0 = computeCorners(box, face, angle)
	if volume ~= nil then
		local volRegion = g_selectioncontext.targetVolumeRegion()
		local rmin = volRegion:mins()
		local rmax = volRegion:maxs()
		local columnTol = sel.getAxis(rmax, wAxis) - sel.getAxis(rmin, wAxis)
		snapCornersToSurface(volume, corners, uAxis, vAxis, wAxis, w0, columnTol, face)
	end
	return corners, face
end

function gizmo(edgeWidth, angle, surfaceTolerance)
	local corners, _ = rectangleCorners(edgeWidth, angle, surfaceTolerance, nil)
	if corners == nil then
		return nil
	end
	local positions = {}
	for i = 1, 4 do
		local c = corners[i]
		positions[i] = {c.x + 0.5, c.y + 0.5, c.z + 0.5}
	end
	positions[5] = positions[1]
	return {
		operations = {"line"},
		positions = positions
	}
end

function select(node, region, edgeWidth, angle, surfaceTolerance)
	local volume = node:volume()
	local modType = g_selectioncontext.modifierType()
	local selecting = modType ~= "erase"

	local corners, face = rectangleCorners(edgeWidth, angle, surfaceTolerance, volume)
	if corners == nil then
		return
	end
	outlineRectangle(volume, corners, face, edgeWidth, selecting)
end
