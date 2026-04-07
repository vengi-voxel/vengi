function arguments()
	return {
		{name = "axis", desc = "Normal axis: auto, x, y, or z", type = "enum", default = "auto", enum = "auto,x,y,z"}
	}
end

function description()
	return "Select a bounded column cross-section (e.g. pillar top or pipe ring)"
end

function icon()
	return "columns3"
end

local sel = require("modules.selection")

function select(node, region, axis)
	local volume = node:volume()
	local seed = g_selectioncontext.aabbFirstPos()
	local face = g_selectioncontext.aabbFace()
	local modType = g_selectioncontext.modifierType()
	local selecting = modType ~= "erase"

	if volume:voxel(seed.x, seed.y, seed.z) == -1 then
		return
	end

	local rmin = region:mins()
	local rmax = region:maxs()

	-- Build probe order: which W axis to try first
	local tryAxes = {}
	if axis ~= "auto" then
		local axisMap = {x = 0, y = 1, z = 2}
		tryAxes[1] = axisMap[axis]
	else
		local faceW = sel.faceToAxisIndex(face)
		tryAxes[1] = faceW
		tryAxes[2] = (faceW + 1) % 3
		tryAxes[3] = (faceW + 2) % 3
	end

	for tryIdx = 1, #tryAxes do
		local wAxis = tryAxes[tryIdx]
		local uAxis = (wAxis + 1) % 3
		local vAxis = (wAxis + 2) % 3

		local offsets = sel.buildPlaneOffsets(uAxis, vAxis)

		local planeW = sel.getAxis(seed, wAxis)

		local solidRegion = {}
		local queue = {}
		local visited = {}
		local seedKey = sel.posKey(seed.x, seed.y, seed.z)
		visited[seedKey] = true
		queue[#queue + 1] = {seed.x, seed.y, seed.z}
		local qfront = 1
		local bounded = true

		while qfront <= #queue do
			local cur = queue[qfront]
			qfront = qfront + 1
			solidRegion[#solidRegion + 1] = cur

			for dir = 1, 4 do
				local nx = cur[1] + offsets[dir][1]
				local ny = cur[2] + offsets[dir][2]
				local nz = cur[3] + offsets[dir][3]
				nx, ny, nz = sel.forceAxis(nx, ny, nz, wAxis, planeW)

				if not sel.containsPoint(rmin, rmax, nx, ny, nz) then
					bounded = false
					break
				end
				local key = sel.posKey(nx, ny, nz)
				if not visited[key] then
					visited[key] = true
					if volume:voxel(nx, ny, nz) ~= -1 then
						queue[#queue + 1] = {nx, ny, nz}
					end
				end
			end
			if not bounded then
				break
			end
		end

		if bounded then
			for _, pos in ipairs(solidRegion) do
				volume:setSelected(pos[1], pos[2], pos[3], selecting)
			end
			return
		end
	end
end
