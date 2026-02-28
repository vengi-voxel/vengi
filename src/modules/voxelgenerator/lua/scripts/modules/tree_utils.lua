local tree_utils = {}

function tree_utils.getCenterBottom(region)
	local pos = region:mins()
	pos.x = pos.x + math.floor(region:width() / 2)
	pos.z = pos.z + math.floor(region:depth() / 2)
	return pos
end

function tree_utils.createL(volume, pos, width, depth, height, thickness, voxel)
	local p = g_ivec3.new(pos.x, pos.y, pos.z)
	if width ~= 0 then
		if width < 0 then
			g_shape.cube(volume, g_ivec3.new(p.x + width, p.y, p.z), -width, thickness, thickness, voxel)
		else
			g_shape.cube(volume, p, width, thickness, thickness, voxel)
		end
		p.x = p.x + width
		g_shape.cube(volume, p, thickness, height, thickness, voxel)
		p.x = p.x + math.floor(thickness / 2)
		p.z = p.z + math.floor(thickness / 2)
	elseif depth ~= 0 then
		if depth < 0 then
			g_shape.cube(volume, g_ivec3.new(p.x, p.y, p.z + depth), thickness, thickness, -depth, voxel)
		else
			g_shape.cube(volume, p, thickness, thickness, depth, voxel)
		end
		p.z = p.z + depth
		g_shape.cube(volume, p, thickness, height, thickness, voxel)
		p.x = p.x + math.floor(thickness / 2)
		p.z = p.z + math.floor(thickness / 2)
	else
		error("width or depth must be != 0")
	end
	p.y = p.y + height
	return p
end

function tree_utils.createTrunk(volume, pos, trunkHeight, trunkStrength, voxel)
	local endPos = g_ivec3.new(pos.x, pos.y + trunkHeight, pos.z)
	g_shape.line(volume, pos, endPos, voxel, trunkStrength)
end

function tree_utils.createBezierTrunk(volume, pos, trunkHeight, trunkWidth, trunkDepth, trunkStrength, trunkControlOffset, trunkFactor, voxel)
	local trunkTop = g_ivec3.new(pos.x, pos.y + trunkHeight, pos.z)
	local shiftX = trunkWidth
	local shiftZ = trunkDepth
	local endPos = g_ivec3.new(trunkTop.x + shiftX, trunkTop.y, trunkTop.z + shiftZ)
	local trunkSize = trunkStrength
	local control = g_ivec3.new(pos.x, pos.y + trunkControlOffset, pos.z)

	local steps = trunkHeight
	local last = pos
	for i = 1, steps do
		local t = i / steps
		local invT = 1.0 - t
		local p = g_ivec3.new(
			math.floor(invT * invT * pos.x + 2 * invT * t * control.x + t * t * endPos.x),
			math.floor(invT * invT * pos.y + 2 * invT * t * control.y + t * t * endPos.y),
			math.floor(invT * invT * pos.z + 2 * invT * t * control.z + t * t * endPos.z)
		)
		g_shape.line(volume, p, last, voxel, math.max(1, math.ceil(trunkSize)))
		trunkSize = trunkSize * trunkFactor
		last = p
	end

	endPos.y = endPos.y - 1
	return endPos
end

function tree_utils.initSeed(seed)
	if seed == 0 then
		math.randomseed(os.time())
	else
		math.randomseed(seed)
	end
end

function tree_utils.drawBezier(volume, startPos, endPos, control, startThickness, endThickness, steps, voxelColor)
	local last = startPos
	for i = 1, steps do
		local t = i / steps
		local invT = 1.0 - t
		local p = g_ivec3.new(
			math.floor(invT * invT * startPos.x + 2 * invT * t * control.x + t * t * endPos.x),
			math.floor(invT * invT * startPos.y + 2 * invT * t * control.y + t * t * endPos.y),
			math.floor(invT * invT * startPos.z + 2 * invT * t * control.z + t * t * endPos.z)
		)
		local thickness = math.max(1, math.ceil(startThickness + (endThickness - startThickness) * t))
		g_shape.line(volume, last, p, voxelColor, thickness)
		last = p
	end
	return last
end

function tree_utils.leafCluster(volume, center, size, leafColor, leafColor2)
	local w = math.max(2, size + math.random(-1, 1))
	local h = math.max(1, math.floor(w * 0.6) + math.random(-1, 0))
	local col = leafColor
	if math.random() > 0.5 then col = leafColor2 end
	g_shape.dome(volume, center, 'y', false, w * 2, h, w * 2, col)
	if size >= 3 then
		g_shape.dome(volume, center, 'y', true,
			math.floor(w * 0.8), math.max(1, h - 1), math.floor(w * 0.8), leafColor)
	end
end

return tree_utils
