--
-- Place LEGO-style studs on exposed top surfaces of existing voxels
--

function arguments()
	return {
		{ name = 'spacing', desc = 'Grid spacing between stud centers', type = 'int', default = '2', min = '1', max = '16' },
		{ name = 'studheight', desc = 'Height of each stud in voxels', type = 'int', default = '1', min = '1', max = '4' },
		{ name = 'studradius', desc = 'Radius of each stud (0 = single voxel)', type = 'int', default = '0', min = '0', max = '3' }
	}
end

function description()
	return "Places LEGO-style studs on exposed surface voxels. Studs inherit the color of the underlying voxel."
end

function main(node, region, color, spacing, studheight, studradius)
	local volume = node:volume()
	local volRegion = volume:region()
	local mins = volRegion:mins()
	local maxs = volRegion:maxs()

	-- Collect stud placement positions before modifying the volume.
	-- A surface voxel is any non-empty voxel with air (empty) directly above it.
	-- We only consider grid-aligned positions so the studs form a regular pattern.
	-- Studs are only placed if there is room within the existing volume bounds.
	local studs = {}
	local maxStudY = maxs.y - studheight + 1

	for x = mins.x, maxs.x, spacing do
		for z = mins.z, maxs.z, spacing do
			for y = mins.y, maxs.y do
				local c = volume:voxel(x, y, z)
				if c ~= -1 then
					local above = volume:voxel(x, y + 1, z)
					if above == -1 and y + 1 <= maxStudY then
						studs[#studs + 1] = { x = x, y = y + 1, z = z, color = c }
					end
				end
			end
		end
	end

	-- Place the studs
	for _, stud in ipairs(studs) do
		if studradius <= 0 then
			-- Single-voxel column stud
			for h = 0, studheight - 1 do
				volume:setVoxel(stud.x, stud.y + h, stud.z, stud.color)
			end
		else
			-- Cylindrical stud
			g_shape.cylinder(volume, g_vec3.new(stud.x, stud.y, stud.z), 'y', studradius, studheight, stud.color)
		end
	end
end
