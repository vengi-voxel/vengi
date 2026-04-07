function description()
	return "Select the closed rim around a hole in a surface"
end

function icon()
	return "circledashed"
end

local sel = require("modules.selection")

function select(node, region)
	local volume = node:volume()
	local seed = g_selectioncontext.aabbFirstPos()
	local face = g_selectioncontext.aabbFace()
	local modType = g_selectioncontext.modifierType()
	local selecting = modType ~= "erase"

	if face == "max" then
		return
	end

	local rmin = region:mins()
	local rmax = region:maxs()

	local wAxis = sel.faceToAxisIndex(face)
	local uAxis = (wAxis + 1) % 3
	local vAxis = (wAxis + 2) % 3

	-- Track successfully processed air regions to avoid re-processing
	local processedAir = {}

	-- Try BFS from a given air seed in a specific 2D plane
	local function tryBfsFromAirSeed(airX, airY, airZ, searchW, searchU, searchV)
		if not sel.containsPoint(rmin, rmax, airX, airY, airZ) then
			return
		end
		if volume:voxel(airX, airY, airZ) ~= -1 then
			return
		end

		local airKey = sel.posKey(airX, airY, airZ)
		if processedAir[airKey] then
			return
		end

		-- Get plane W coordinate
		local planeW
		if searchW == 0 then
			planeW = airX
		elseif searchW == 1 then
			planeW = airY
		else
			planeW = airZ
		end

		local offsets = sel.buildPlaneOffsets(searchU, searchV)

		local airRegion = {}
		local queue = {}
		local visited = {}
		visited[airKey] = true
		queue[1] = {airX, airY, airZ}
		local qfront = 1
		local bounded = true

		while qfront <= #queue do
			local cur = queue[qfront]
			qfront = qfront + 1
			airRegion[#airRegion + 1] = cur

			for dir = 1, 4 do
				local nx = cur[1] + offsets[dir][1]
				local ny = cur[2] + offsets[dir][2]
				local nz = cur[3] + offsets[dir][3]
				nx, ny, nz = sel.forceAxis(nx, ny, nz, searchW, planeW)

				if not sel.containsPoint(rmin, rmax, nx, ny, nz) then
					bounded = false
					break
				end
				local key = sel.posKey(nx, ny, nz)
				if not visited[key] then
					visited[key] = true
					-- Only expand through air
					if volume:voxel(nx, ny, nz) == -1 then
						queue[#queue + 1] = {nx, ny, nz}
					end
				end
			end
			if not bounded then
				break
			end
		end

		if not bounded then
			-- Don't mark processedAir so other axis planes can retry
			return
		end

		-- Mark all air voxels in the region as processed
		for _, pos in ipairs(airRegion) do
			processedAir[sel.posKey(pos[1], pos[2], pos[3])] = true
		end

		-- Select solid voxels adjacent to the bounded air region
		for _, pos in ipairs(airRegion) do
			for dir = 1, 4 do
				local nx = pos[1] + offsets[dir][1]
				local ny = pos[2] + offsets[dir][2]
				local nz = pos[3] + offsets[dir][3]
				nx, ny, nz = sel.forceAxis(nx, ny, nz, searchW, planeW)

				if sel.containsPoint(rmin, rmax, nx, ny, nz) then
					if volume:voxel(nx, ny, nz) ~= -1 then
						volume:setSelected(nx, ny, nz, selecting)
					end
				end
			end
		end
	end

	-- UV neighbor offsets
	local uvOffsets = sel.buildPlaneOffsets(uAxis, vAxis)

	-- Seed type 1: 4 UV neighbors at same W depth as clicked voxel
	for startDir = 1, 4 do
		local ax = seed.x + uvOffsets[startDir][1]
		local ay = seed.y + uvOffsets[startDir][2]
		local az = seed.z + uvOffsets[startDir][3]
		tryBfsFromAirSeed(ax, ay, az, wAxis, uAxis, vAxis)
	end

	-- Seed type 2: air voxel one step in face direction
	local faceSign = sel.isPositiveFace(face) and 1 or -1
	local fx, fy, fz = seed.x, seed.y, seed.z
	if wAxis == 0 then
		fx = fx + faceSign
	elseif wAxis == 1 then
		fy = fy + faceSign
	else
		fz = fz + faceSign
	end

	-- Try all 3 axis planes through the face seed
	for altW = 0, 2 do
		local altU = (altW + 1) % 3
		local altV = (altW + 2) % 3
		tryBfsFromAirSeed(fx, fy, fz, altW, altU, altV)
	end
end
