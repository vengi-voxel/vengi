local vol = require "modules.volume"

function arguments()
	return {}
end

local function randomUnvisitedNeighbour(volume, x, y, z, visited)
	-- visit 4 directions
	local offset = {
		{ 0, -1, 1, 0},
		{ 0,  1, 1, 0},
		{-1,  0, 0, 1},
		{ 1,  0, 0, 1}
	}
	local step = math.random(1,4)
	for i = 1, 4 do
		local index = (i + step) % 4 + 1
		local dx = offset[index][1]
		local dz = offset[index][2]
		local x2 = x + dx
		local z2 = z + dz
		if volume:voxel(x2, y, z2) == 0 and visited[x2..':'..z2] == nil then
			local x3 = x2 + dx
			local z3 = z2 + dz
			if volume:voxel(x3, y, z3) == 0 and visited[x3..':'..z3] == nil then
				return x2, z2, offset[index][3], offset[index][4]
			end
		end
	end
	return nil, nil, nil, nil
end

local function randomizeDFS(volume, x, y, z, visited)
	visited[x..':'..z] = 1
	local x2, z2, dx, dz = randomUnvisitedNeighbour(volume, x, y, z, visited)
	-- mark two neighnours as visited, too
	if dx == 1 then
		local dir = math.random(1,3)
		if dir == 1 then
			visited[(x2 + dx)..':'..z2] = 1
		elseif dir == 2 then
			visited[(x2 + dx)..':'..z2] = 1
			visited[(x2 - dx)..':'..z2] = 1
		else
			visited[(x2 - dx)..':'..z2] = 1
		end
	elseif dz == 1 then
		local dir = math.random(1,3)
		if dir == 1 then
			visited[x2..':'..(z2 + dz)] = 1
		elseif dir == 2 then
			visited[x2..':'..(z2 + dz)] = 1
			visited[x2..':'..(z2 - dz)] = 1
		else
			visited[x2..':'..(z2 - dz)] = 1
		end
	end
	while x2 ~= nil do
		volume:setVoxel(x2, y, z2, -1)
		randomizeDFS(volume, x2, y, z2, visited)
		x2, z2 = randomUnvisitedNeighbour(volume, x2, y, z2, visited)
	end
end

function main(node, region, color)
	local mins = region:mins()
	local fill = function (volume, x, z)
		volume:setVoxel(x, mins.y, z, color)
	end
	vol.visitXZ(node:volume(), region, fill)

	local visited = {}
	randomizeDFS(node:volume(), mins.x + 1, mins.y, mins.z + 1, visited)
end
