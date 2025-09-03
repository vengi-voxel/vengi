local module = {}

---
--- visit all voxels of a volume in the given region
--- and call the visitor function for each voxel if the
--- condition function returns true
---
--- the order of visit is first y axis, then x axis, then z axis
---
--- See also the visit functions where you don't have to specify the condition
---
function module.conditionYXZ(volume, region, visitor, condition)
	local mins = region:mins()
	local maxs = region:maxs()
	tracy.ZoneBegin()
	local yieldCounter = 32*32*32
	for y = mins.y, maxs.y do
		for x = mins.x, maxs.x do
			for z = mins.z, maxs.z do
				if condition(volume, x, y, z) then
					visitor(volume, x, y, z)
				end
				if yieldCounter == 0 then
					coroutine.yield()
					yieldCounter = 32*32*32
				end
				yieldCounter = yieldCounter - 1
			end
		end
	end
	tracy.ZoneEnd()
end

---
--- visit all x and z positions in the given region
---
--- See also the visit functions where you don't have to specify the condition
---
function module.conditionXZ(volume, region, visitor, condition)
	local mins = region:mins()
	local maxs = region:maxs()
	local yieldCounter = 32*32*32
	for x = mins.x, maxs.x do
		for z = mins.z, maxs.z do
			if condition(volume, x, z) then
				visitor(volume, x, z)
			end
			if yieldCounter == 0 then
				coroutine.yield()
				yieldCounter = 32*32*32
			end
			yieldCounter = yieldCounter - 1
		end
	end
end

---
--- See also the visit functions where you don't have to specify the condition
---
function module.conditionYXZDown(volume, region, visitor, condition)
	local mins = region:mins()
	local maxs = region:maxs()
	local yieldCounter = 32*32*32
	for y = maxs.y, mins.y, -1 do
		for x = mins.x, maxs.x do
			for z = mins.z, maxs.z do
				if condition(volume, x, y, z) then
					visitor(volume, x, y, z)
				end
				if yieldCounter == 0 then
					coroutine.yield()
					yieldCounter = 32*32*32
				end
				yieldCounter = yieldCounter - 1
			end
		end
	end
end

---
--- Count the amount of empty voxels around the given position
---
function module.countEmptyAround(volume, x, y, z, size)
	local adjacent = 0
	for sx = -size, size, 1 do
		if (size > 2) then
			coroutine.yield()
		end
		for sy = -size, size, 1 do
			for sz = -size, size, 1 do
				if (sx ~= 0 or sy ~= 0 or sz ~= 0) then
					local color = volume:voxel(x + sx, y + sy, z + sz)
					-- if empty voxel
					if color == -1 then
						adjacent = adjacent + 1
					end
				end
			end
		end
	end
	return adjacent
end

function module.countEmptyAroundOnY(volume, x, y, z, size)
	local adjacent = 0
	for sx = -size, size, 1 do
		if (size > 2) then
			coroutine.yield()
		end
		for sz = -size, size, 1 do
			if (sx ~= 0 or sz ~= 0) then
				local color = volume:voxel(x + sx, y, z + sz)
				-- if empty voxel
				if color == -1 then
					adjacent = adjacent + 1
				end
			end
		end
	end
	return adjacent
end

---
--- See also the condition functions where you can also specify the condition
---
function module.visitYXZDown(volume, region, visitor)
	local condition = function(x, y, z)
		return true
	end
	module.conditionYXZDown(volume, region, visitor, condition)
end

---
--- See also the condition functions where you can also specify the condition
---
function module.visitYXZ(volume, region, visitor)
	local condition = function(x, y, z)
		return true
	end
	module.conditionYXZ(volume, region, visitor, condition)
end

---
--- See also the condition functions where you can also specify the condition
---
function module.visitXZ(volume, region, visitor)
	local condition = function (x, z) return true end
	module.conditionXZ(volume, region, visitor, condition)
end

--- visit directions
local offset = {
	{ 0,  0, -1},
	{ 0,  0,  1},
	{ 0, -1,  0},
	{ 0,  1,  0},
	{-1,  0,  0},
	{ 1,  0,  0}, -- faces
	{ 0, -1, -1},
	{ 0, -1,  1},
	{ 0,  1, -1},
	{ 0,  1,  1},
	{-1,  0, -1},
	{-1,  0,  1},
	{ 1,  0, -1},
	{ 1,  0,  1},
	{-1, -1,  0},
	{-1,  1,  0},
	{ 1, -1,  0},
	{ 1,  1,  0}, -- edges
	{-1, -1, -1},
	{-1, -1,  1},
	{-1,  1, -1},
	{-1,  1,  1},
	{ 1, -1, -1},
	{ 1, -1,  1},
	{ 1,  1, -1},
	{ 1,  1,  1} -- corners
}

local function visitConnectedInternal(volume, x, y, z, visitor, visited, directions)
	if volume:voxel(x, y, z) == -1 then
		return
	end

	for i = 1, directions do
		local x2 = x + offset[i][1]
		local y2 = y + offset[i][2]
		local z2 = z + offset[i][3]
		-- if voxel is not empty and not visited yet
		if volume:voxel(x2, y2, z2) ~= -1 then
			if visited[x2 .. ":" .. y2 .. ":" .. z2] == nil then
				visited[x2 .. ":" .. y2 .. ":" .. z2] = 1
				-- recurse into the connected voxels of this voxel, too
				visitConnectedInternal(volume, x2, y2, z2, visitor, visited, directions)
				-- call the user given visitor
				visitor(volume, x2, y2, z2)
			end
		end
	end
end

---
--- Visit non-empty connected voxels in 6 directions - all those that are sharing a face
---
function module.visitConnected6(volume, x, y, z, visitor)
	-- remember visited voxels in this table
	local visited = {}
	visitConnectedInternal(volume, x, y, z, visitor, visited, 6)
end

---
--- Visit non-empty connected voxels in 18 directions - all those that are sharing a face and an edge
---
function module.visitConnected18(volume, x, y, z, visitor)
	-- remember visited voxels in this table
	local visited = {}
	visitConnectedInternal(volume, x, y, z, visitor, visited, 18)
end

---
--- Visit non-empty connected voxels in 26 directions - all those that are sharing a face, edge, or corner
---
function module.visitConnected26(volume, x, y, z, visitor)
	-- remember visited voxels in this table
	local visited = {}
	visitConnectedInternal(volume, x, y, z, visitor, visited, 26)
end

function module.replaceColor(node, region, oldColor, newColor)
	local visitor = function (volume, x, y, z)
		volume:setVoxel(x, y, z, newColor)
	end

	local condition = function (volume, x, y, z)
		local voxel = volume:voxel(x, y, z)
		if voxel == oldColor then
			return true
		end
		return false
	end
	module.conditionYXZ(node:volume(), region, visitor, condition)
end

return module
