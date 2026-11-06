local module = {}

--- Calculate a sphere region centered at cursor with given radius
--- @param cx number cursor x
--- @param cy number cursor y
--- @param cz number cursor z
--- @param radius number sphere radius
--- @return number, number, number, number, number, number min/max bounds
function module.sphereRegion(cx, cy, cz, radius)
	return cx - radius, cy - radius, cz - radius, cx + radius, cy + radius, cz + radius
end

--- Place voxels in a filled sphere
--- @param volume userdata volume to modify
--- @param cx number center x
--- @param cy number center y
--- @param cz number center z
--- @param radius number sphere radius
--- @param color number voxel color index
function module.fillSphere(volume, cx, cy, cz, radius, color)
	local r2 = radius * radius
	for x = -radius, radius do
		for y = -radius, radius do
			for z = -radius, radius do
				if x * x + y * y + z * z <= r2 then
					volume:setVoxel(cx + x, cy + y, cz + z, color)
				end
			end
		end
	end
end

--- Place voxels in a hollow sphere shell
--- @param volume userdata volume to modify
--- @param cx number center x
--- @param cy number center y
--- @param cz number center z
--- @param outerRadius number outer sphere radius
--- @param innerRadius number inner sphere radius (empty inside)
--- @param color number voxel color index
function module.fillSphericalShell(volume, cx, cy, cz, outerRadius, innerRadius, color)
	local outerR2 = outerRadius * outerRadius
	local innerR2 = innerRadius * innerRadius
	for x = -outerRadius, outerRadius do
		for y = -outerRadius, outerRadius do
			for z = -outerRadius, outerRadius do
				local d2 = x * x + y * y + z * z
				if d2 <= outerR2 and d2 >= innerR2 then
					volume:setVoxel(cx + x, cy + y, cz + z, color)
				end
			end
		end
	end
end

--- Fill a box region with voxels
--- @param volume userdata volume to modify
--- @param region userdata region defining the box bounds
--- @param color number voxel color index
function module.fillBox(volume, region, color)
	local mins = region:mins()
	local maxs = region:maxs()
	for x = mins.x, maxs.x do
		for y = mins.y, maxs.y do
			for z = mins.z, maxs.z do
				volume:setVoxel(x, y, z, color)
			end
		end
	end
end

--- Calculate the squared distance between two 3D points
--- @param x1 number
--- @param y1 number
--- @param z1 number
--- @param x2 number
--- @param y2 number
--- @param z2 number
--- @return number squared distance
function module.distanceSquared(x1, y1, z1, x2, y2, z2)
	local dx = x2 - x1
	local dy = y2 - y1
	local dz = z2 - z1
	return dx * dx + dy * dy + dz * dz
end

--- Fill a cylinder along a given axis
--- @param volume userdata volume to modify
--- @param cx number center x
--- @param cy number center y
--- @param cz number center z
--- @param radius number cylinder radius
--- @param height number cylinder height
--- @param axis string "x", "y", or "z"
--- @param color number voxel color index
function module.fillCylinder(volume, cx, cy, cz, radius, height, axis, color)
	local r2 = radius * radius
	local halfHeight = math.floor(height / 2)
	for h = -halfHeight, halfHeight do
		for a = -radius, radius do
			for b = -radius, radius do
				if a * a + b * b <= r2 then
					if axis == "y" then
						volume:setVoxel(cx + a, cy + h, cz + b, color)
					elseif axis == "x" then
						volume:setVoxel(cx + h, cy + a, cz + b, color)
					else
						volume:setVoxel(cx + a, cy + b, cz + h, color)
					end
				end
			end
		end
	end
end

return module
