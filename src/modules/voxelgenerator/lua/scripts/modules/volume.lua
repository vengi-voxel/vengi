local module = {}

function module.conditionYXZ(volume, region, visitor, condition)
	local mins = region:mins()
	local maxs = region:maxs()
	for y = mins.y, maxs.y do
		for x = mins.x, maxs.x do
			for z = mins.z, maxs.z do
				if condition(volume, x, y, z) then
					visitor(volume, x, y, z)
				end
			end
		end
	end
end

function module.conditionXZ(volume, region, visitor, condition)
	local mins = region:mins()
	local maxs = region:maxs()
	for x = mins.x, maxs.x do
		for z = mins.z, maxs.z do
			if condition(volume, x, z) then
				visitor(volume, x, z)
			end
		end
	end
end

function module.conditionYXZDown(volume, region, visitor, condition)
	local mins = region:mins()
	local maxs = region:maxs()
	for y = maxs.y, mins.y, -1 do
		for x = mins.x, maxs.x do
			for z = mins.z, maxs.z do
				if condition(volume, x, y, z) then
					visitor(volume, x, y, z)
				end
			end
		end
	end
end

function module.countEmptyAround(volume, x, y, z)
	local adjacent = 0
	for sx = -1, 1, 1 do
		for sy = -1, 1, 1 do
			for sz = -1, 1, 1 do
				if (sx ~= 0 or sy ~= 0 or sz ~= 0) then
					local color = volume:voxel(x + sx, y + sy, z + sz)
					if color == -1 then
						adjacent = adjacent + 1
					end
				end
			end
		end
	end
	return adjacent
end

function module.visitYXZDown(volume, region, visitor)
	local condition = function (x, y, z) return true end
	module.conditionYXZDown(volume, region, visitor, condition)
end

function module.visitYXZ(volume, region, visitor)
	local condition = function (x, y, z) return true end
	module.conditionYXZ(volume, region, visitor, condition)
end

function module.visitXZ(volume, region, visitor)
	local condition = function (x, z) return true end
	module.conditionXZ(volume, region, visitor, condition)
end

local function visitConnectedInternal(volume, x, y, z, visitor, visited)
	if volume:voxel(x, y, z) == -1 then
		return
	end

	-- visit 6 directions
	local offset = {
		{ 0,  0, -1},
		{ 0,  0,  1},
		{ 0, -1,  0},
		{ 0,  1,  0},
		{-1,  0,  0},
		{ 1,  0,  0}
	}
	for i=1,6 do
		local x2 = x + offset[i][1]
		local y2 = y + offset[i][2]
		local z2 = z + offset[i][3]
		local key = x2..':'..y2..':'..z2
		local v = volume:voxel(x2, y2, z2)
		if v ~= -1 then
			if visited[key] == nil then
				visited[key] = 1
				visitConnectedInternal(volume, x2, y2, z2, visitor, visited)
				visitor(volume, x2, y2, z2)
			end
		end
	end
end

function module.visitConnected(volume, x, y, z, visitor)
	local visited = {}
	visitConnectedInternal(volume, x, y, z, visitor, visited)
end

return module
