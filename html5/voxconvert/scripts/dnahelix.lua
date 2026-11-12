--
-- DNA double helix generator
-- Builds two sugar-phosphate backbones winding around a shared axis, with
-- complementary A-T and G-C base-pair rungs. Fits the current node region.
-- Classic educational coloring: A green, T red, G yellow, C blue.
--

function arguments()
	return {
		{ name = 'turns', desc = 'Number of helical turns', type = 'float', default = '2.5', min = '0.25', max = '20.0' },
		{ name = 'radius', desc = 'Helix radius in voxels (0 = fit to region)', type = 'int', default = '0', min = '0', max = '64' },
		{ name = 'strandThickness', desc = 'Backbone strand thickness', type = 'int', default = '1', min = '1', max = '5' },
		{ name = 'basePairThickness', desc = 'Base pair rung thickness', type = 'int', default = '1', min = '1', max = '4' },
		{ name = 'pairsPerTurn', desc = 'Base pairs per turn (B-DNA is about 10)', type = 'int', default = '10', min = '2', max = '20' },
		{ name = 'strandOffset', desc = 'Offset between strands in degrees', type = 'int', default = '180', min = '90', max = '180' },
		{ name = 'handedness', desc = 'Helix handedness (B-DNA is right)', type = 'enum', default = 'right', enum = 'right,left' },
		{ name = 'showBasePairs', desc = 'Draw complementary base-pair rungs', type = 'bool', default = 'true' },
		{ name = 'gcContent', desc = 'G-C pair percentage (rest are A-T)', type = 'int', default = '50', min = '0', max = '100' },
		{ name = 'backboneColor1', desc = 'First strand backbone color', type = 'hexcolor', default = '#4FC3F7' },
		{ name = 'backboneColor2', desc = 'Second strand backbone color', type = 'hexcolor', default = '#CE93D8' },
		{ name = 'adenineColor', desc = 'Adenine (A) color', type = 'hexcolor', default = '#43A047' },
		{ name = 'thymineColor', desc = 'Thymine (T) color', type = 'hexcolor', default = '#E53935' },
		{ name = 'guanineColor', desc = 'Guanine (G) color', type = 'hexcolor', default = '#FDD835' },
		{ name = 'cytosineColor', desc = 'Cytosine (C) color', type = 'hexcolor', default = '#1E88E5' },
		{ name = 'seed', desc = 'Random seed for base sequence (0 = random)', type = 'int', default = '0' }
	}
end

function description()
	return 'Generates a DNA double helix with colored complementary base pairs in the current region'
end

local function round(v)
	return math.floor(v + 0.5)
end

local function setVoxel(volume, x, y, z, c)
	x = round(x)
	y = round(y)
	z = round(z)
	if volume:containsPoint(x, y, z) then
		volume:setVoxel(x, y, z, c)
	end
end

local function setBlob(volume, x, y, z, thickness, c)
	local r = thickness - 1
	if r <= 0 then
		setVoxel(volume, x, y, z, c)
		return
	end
	local r2 = r * r + 0.25
	for dy = -r, r do
		for dx = -r, r do
			for dz = -r, r do
				if dx * dx + dy * dy + dz * dz <= r2 then
					setVoxel(volume, x + dx, y + dy, z + dz, c)
				end
			end
		end
	end
end

local function voxelLine(volume, x1, y1, z1, x2, y2, z2, thickness, c)
	local dx = x2 - x1
	local dy = y2 - y1
	local dz = z2 - z1
	local len = math.sqrt(dx * dx + dy * dy + dz * dz)
	local n = math.max(1, math.ceil(len * 2))
	for i = 0, n do
		local t = i / n
		setBlob(volume, x1 + dx * t, y1 + dy * t, z1 + dz * t, thickness, c)
	end
end

local function helixPoint(cx, y0, cz, radius, t, height, turns, offset, sign)
	local theta = sign * t * turns * 2.0 * math.pi + offset
	local x = cx + radius * math.cos(theta)
	local y = y0 + t * (height - 1)
	local z = cz + radius * math.sin(theta)
	return x, y, z
end

local function pickPair(gcContent, adenineColor, thymineColor, guanineColor, cytosineColor)
	local gc = math.random() * 100.0 < gcContent
	local flip = math.random() < 0.5
	if gc then
		if flip then
			return guanineColor, cytosineColor
		end
		return cytosineColor, guanineColor
	end
	if flip then
		return adenineColor, thymineColor
	end
	return thymineColor, adenineColor
end

function main(node, region, color, turns, radius, strandThickness, basePairThickness, pairsPerTurn,
	strandOffset, handedness, showBasePairs, gcContent, backboneColor1, backboneColor2,
	adenineColor, thymineColor, guanineColor, cytosineColor, seed)

	if seed == 0 then
		math.randomseed(os.time())
	else
		math.randomseed(seed)
	end

	local volume = node:volume()
	local mins = region:mins()
	local maxs = region:maxs()
	local size = region:size()
	local height = region:height()
	if height < 1 then
		return
	end

	local cx = math.floor((mins.x + maxs.x) / 2)
	local y0 = mins.y
	local cz = math.floor((mins.z + maxs.z) / 2)

	local helixRadius = radius
	if helixRadius == 0 then
		helixRadius = math.floor(math.min(size.x, size.z) / 2) - strandThickness
	end
	if helixRadius < 1 then
		helixRadius = 1
	end

	local sign = 1
	if handedness == 'left' then
		sign = -1
	end
	local offset2 = strandOffset * math.pi / 180.0

	-- Sample densely enough that consecutive backbone voxels stay connected
	local pitch = height / math.max(turns, 0.25)
	local circumference = 2.0 * math.pi * helixRadius
	local arcLen = turns * math.sqrt(circumference * circumference + pitch * pitch)
	local steps = math.max(height * 4, math.ceil(arcLen * 2), 8)

	local prev1x, prev1y, prev1z
	local prev2x, prev2y, prev2z
	for i = 0, steps do
		local t = i / steps
		local x1, y1, z1 = helixPoint(cx, y0, cz, helixRadius, t, height, turns, 0.0, sign)
		local x2, y2, z2 = helixPoint(cx, y0, cz, helixRadius, t, height, turns, offset2, sign)
		if i > 0 then
			voxelLine(volume, prev1x, prev1y, prev1z, x1, y1, z1, strandThickness, backboneColor1)
			voxelLine(volume, prev2x, prev2y, prev2z, x2, y2, z2, strandThickness, backboneColor2)
		else
			setBlob(volume, x1, y1, z1, strandThickness, backboneColor1)
			setBlob(volume, x2, y2, z2, strandThickness, backboneColor2)
		end
		prev1x, prev1y, prev1z = x1, y1, z1
		prev2x, prev2y, prev2z = x2, y2, z2
	end

	if not showBasePairs then
		return
	end

	local totalPairs = math.max(1, math.floor(turns * pairsPerTurn + 0.5))
	-- Keep at least one voxel of spacing so rungs stay distinct
	local maxPairs = math.max(1, height)
	if totalPairs > maxPairs then
		totalPairs = maxPairs
	end

	for p = 0, totalPairs - 1 do
		local t
		if totalPairs == 1 then
			t = 0.5
		else
			t = p / (totalPairs - 1)
		end
		local x1, y1, z1 = helixPoint(cx, y0, cz, helixRadius, t, height, turns, 0.0, sign)
		local x2, y2, z2 = helixPoint(cx, y0, cz, helixRadius, t, height, turns, offset2, sign)
		local c1, c2 = pickPair(gcContent, adenineColor, thymineColor, guanineColor, cytosineColor)

		local mx = (x1 + x2) * 0.5
		local my = (y1 + y2) * 0.5
		local mz = (z1 + z2) * 0.5

		-- Split the rung so each half shows one complementary base
		voxelLine(volume, x1, y1, z1, mx, my, mz, basePairThickness, c1)
		voxelLine(volume, mx, my, mz, x2, y2, z2, basePairThickness, c2)

		-- Attachment blobs where the bases meet the backbone
		setBlob(volume, x1, y1, z1, strandThickness, backboneColor1)
		setBlob(volume, x2, y2, z2, strandThickness, backboneColor2)
	end
end
