function description()
	return "Select the minimal closed rim of a 3D hole"
end

function icon()
	return "circledashed"
end

local sel = require("modules.selection")

local MAX_SEARCH_RADIUS = 64
local MIN_CYCLE_LEN = 3

-- 6 face neighbor offsets
local FACE_OFFSETS = {
	{0, 0, -1},
	{0, 0, 1},
	{0, -1, 0},
	{0, 1, 0},
	{-1, 0, 0},
	{1, 0, 0}
}

function select(node, region)
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

	-- Check if voxel is solid with at least one air face neighbor (surface voxel)
	local function isSurface(x, y, z)
		if not sel.containsPoint(rmin, rmax, x, y, z) then
			return false
		end
		if volume:voxel(x, y, z) == -1 then
			return false
		end
		for _, off in ipairs(FACE_OFFSETS) do
			local nx, ny, nz = x + off[1], y + off[2], z + off[3]
			if sel.containsPoint(rmin, rmax, nx, ny, nz) and volume:voxel(nx, ny, nz) == -1 then
				return true
			end
		end
		return false
	end

	-- Gate: clicked voxel must have at least one 6-face air neighbor
	local faceSign = sel.isPositiveFace(face) and 1 or -1
	local faceAxisIdx = sel.faceToAxisIndex(face)
	local found = false
	do
		local px, py, pz = seed.x, seed.y, seed.z
		if faceAxisIdx == 0 then
			px = px + faceSign
		elseif faceAxisIdx == 1 then
			py = py + faceSign
		else
			pz = pz + faceSign
		end
		if sel.containsPoint(rmin, rmax, px, py, pz) and volume:voxel(px, py, pz) == -1 then
			found = true
		end
		if not found then
			for _, off in ipairs(FACE_OFFSETS) do
				local nx, ny, nz = seed.x + off[1], seed.y + off[2], seed.z + off[3]
				if sel.containsPoint(rmin, rmax, nx, ny, nz) and volume:voxel(nx, ny, nz) == -1 then
					found = true
					break
				end
			end
		end
	end
	if not found then
		return
	end

	-- BFS on surface graph (face-connected only)
	-- Parallel arrays indexed by node ID
	local bfsPos = {} -- {x, y, z}
	local bfsParent = {} -- parent index
	local bfsDist = {} -- distance from root
	local posToIdx = {} -- posKey -> index

	-- Root node
	bfsPos[1] = {seed.x, seed.y, seed.z}
	bfsParent[1] = 1
	bfsDist[1] = 0
	posToIdx[sel.posKey(seed.x, seed.y, seed.z)] = 1

	-- Non-tree edges
	local nonTreeEdges = {}

	local qi = 0
	while qi < #bfsPos do
		qi = qi + 1
		local cur = bfsPos[qi]
		local curDist = bfsDist[qi]
		if curDist < MAX_SEARCH_RADIUS then
			for _, off in ipairs(FACE_OFFSETS) do
				local nx, ny, nz = cur[1] + off[1], cur[2] + off[2], cur[3] + off[3]
				if isSurface(nx, ny, nz) then
					local key = sel.posKey(nx, ny, nz)
					local nbIdx = posToIdx[key]
					if nbIdx == nil then
						-- New node: add to BFS tree
						local newIdx = #bfsPos + 1
						posToIdx[key] = newIdx
						bfsPos[newIdx] = {nx, ny, nz}
						bfsParent[newIdx] = qi
						bfsDist[newIdx] = curDist + 1
					else
						-- Already visited: potential non-tree edge
						if nbIdx < qi and nbIdx ~= bfsParent[qi] then
							-- Skip back-edges (ancestor in BFS tree)
							local distDiff = curDist - bfsDist[nbIdx]
							local walkIdx = qi
							for _ = 1, distDiff do
								walkIdx = bfsParent[walkIdx]
							end
							if walkIdx ~= nbIdx then
								local cycleLen = curDist + 1 + bfsDist[nbIdx]
								if cycleLen >= MIN_CYCLE_LEN then
									nonTreeEdges[#nonTreeEdges + 1] = {
										uIdx = qi,
										vIdx = nbIdx,
										cycleLen = cycleLen
									}
								end
							end
						end
					end
				end
			end
		end
	end

	if #nonTreeEdges == 0 then
		return
	end

	-- Trace path from node to root
	local function tracePath(nodeIdx)
		local path = {}
		local idx = nodeIdx
		while true do
			path[#path + 1] = bfsPos[idx]
			if idx == 1 then
				break
			end
			idx = bfsParent[idx]
		end
		return path
	end

	-- Process non-tree edges from shortest cycle to longest
	while #nonTreeEdges > 0 do
		-- Find minimum cycle length
		local minIdx = 1
		for i = 2, #nonTreeEdges do
			if nonTreeEdges[i].cycleLen < nonTreeEdges[minIdx].cycleLen then
				minIdx = i
			end
		end
		local edge = nonTreeEdges[minIdx]
		-- Swap-and-pop removal
		nonTreeEdges[minIdx] = nonTreeEdges[#nonTreeEdges]
		nonTreeEdges[#nonTreeEdges] = nil

		local pathU = tracePath(edge.uIdx)
		local pathV = tracePath(edge.vIdx)

		-- Build cycle: reverse pathU [S,...,u] + pathV [v,...,before_S]
		local cycle = {}
		for i = #pathU, 1, -1 do
			cycle[#cycle + 1] = pathU[i]
		end
		for i = 1, #pathV - 1 do
			cycle[#cycle + 1] = pathV[i]
		end

		-- Reject duplicate positions (figure-8 cycles)
		local cycleSet = {}
		local hasDuplicate = false
		for _, cp in ipairs(cycle) do
			local key = sel.posKey(cp[1], cp[2], cp[3])
			if cycleSet[key] then
				hasDuplicate = true
				break
			end
			cycleSet[key] = true
		end
		if not hasDuplicate then
			-- Determine projection plane from cycle planarity
			local minC = {cycle[1][1], cycle[1][2], cycle[1][3]}
			local maxC = {cycle[1][1], cycle[1][2], cycle[1][3]}
			for _, cv in ipairs(cycle) do
				for ax = 1, 3 do
					if cv[ax] < minC[ax] then
						minC[ax] = cv[ax]
					end
					if cv[ax] > maxC[ax] then
						maxC[ax] = cv[ax]
					end
				end
			end
			local spread = {maxC[1] - minC[1], maxC[2] - minC[2], maxC[3] - minC[3]}
			local cycleFaceAx = faceAxisIdx -- 0-based
			for ax = 0, 2 do
				if spread[ax + 1] < spread[cycleFaceAx + 1] then
					cycleFaceAx = ax
				end
			end
			local cycleUAx = (cycleFaceAx + 1) % 3
			local cycleVAx = (cycleFaceAx + 2) % 3

			-- Compute centroid
			local centU, centV, centDepth = 0, 0, 0
			for _, cv in ipairs(cycle) do
				centU = centU + sel.getAxisArray(cv, cycleUAx)
				centV = centV + sel.getAxisArray(cv, cycleVAx)
				centDepth = centDepth + sel.getAxisArray(cv, cycleFaceAx)
			end
			local n = #cycle
			centU = math.floor(centU / n + 0.5)
			centV = math.floor(centV / n + 0.5)
			centDepth = math.floor(centDepth / n + 0.5)

			-- Build path table with x,y,z fields for lassoContains
			local pathTable = {}
			for i, cv in ipairs(cycle) do
				pathTable[i] = {x = cv[1], y = cv[2], z = cv[3]}
			end

			if volume:lassoContains(pathTable, centU, centV, cycleUAx, cycleVAx) then
				-- Hole check: centroid must be air
				local testCoords = {0, 0, 0}
				testCoords[cycleFaceAx + 1] = centDepth
				testCoords[cycleUAx + 1] = centU
				testCoords[cycleVAx + 1] = centV
				local testX, testY, testZ = testCoords[1], testCoords[2], testCoords[3]

				if sel.containsPoint(rmin, rmax, testX, testY, testZ) and volume:voxel(testX, testY, testZ) == -1 then
					-- Valid rim found: select all cycle voxels
					for _, pos in ipairs(cycle) do
						if volume:voxel(pos[1], pos[2], pos[3]) ~= -1 then
							volume:setSelected(pos[1], pos[2], pos[3], selecting)
						end
					end
					return
				end
			end
		end
	end
end
