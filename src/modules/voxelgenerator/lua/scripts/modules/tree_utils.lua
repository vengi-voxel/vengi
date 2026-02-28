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
		local p = tree_utils.bezierPointAt(pos, endPos, control, t)
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

function tree_utils.bezierPointAt(startPos, endPos, control, t)
	local invT = 1.0 - t
	return g_ivec3.new(
		math.floor(invT * invT * startPos.x + 2 * invT * t * control.x + t * t * endPos.x),
		math.floor(invT * invT * startPos.y + 2 * invT * t * control.y + t * t * endPos.y),
		math.floor(invT * invT * startPos.z + 2 * invT * t * control.z + t * t * endPos.z)
	)
end

function tree_utils.drawBezier(volume, startPos, endPos, control, startThickness, endThickness, steps, voxelColor)
	local last = startPos
	for i = 1, steps do
		local t = i / steps
		local p = tree_utils.bezierPointAt(startPos, endPos, control, t)
		local thickness = math.max(1, math.ceil(startThickness + (endThickness - startThickness) * t))
		g_shape.line(volume, last, p, voxelColor, thickness)
		last = p
	end
	return last
end

function tree_utils.createCurvedTrunk(volume, basePos, height, strength, curve, topThickness, voxelColor, ctrlFactor)
	ctrlFactor = ctrlFactor or 0.4
	local curveDir = math.random() * 2 * math.pi
	local curveX = math.floor(math.cos(curveDir) * curve)
	local curveZ = math.floor(math.sin(curveDir) * curve)
	local topPos = g_ivec3.new(basePos.x + curveX, basePos.y + height, basePos.z + curveZ)
	local ctrl = g_ivec3.new(
		basePos.x + math.floor(curveX * ctrlFactor),
		basePos.y + math.floor(height * 0.5),
		basePos.z + math.floor(curveZ * ctrlFactor)
	)
	tree_utils.drawBezier(volume, basePos, topPos, ctrl, strength, topThickness, height, voxelColor)
	return topPos, topThickness, ctrl
end

function tree_utils.createBezierRoots(volume, basePos, numRoots, rootLength, rootThickness, voxelColor)
	if numRoots <= 0 then return end
	local angleStep = (2 * math.pi) / numRoots
	local startAngle = math.random() * 2 * math.pi
	for i = 1, numRoots do
		local angle = startAngle + (i - 1) * angleStep + (math.random() - 0.5) * 0.5
		local dx = math.cos(angle)
		local dz = math.sin(angle)
		local startP = g_ivec3.new(basePos.x, basePos.y, basePos.z)
		local endP = g_ivec3.new(
			math.floor(basePos.x + dx * rootLength),
			basePos.y - math.random(0, 1),
			math.floor(basePos.z + dz * rootLength)
		)
		local ctrl = g_ivec3.new(
			math.floor(basePos.x + dx * rootLength * 0.5),
			basePos.y + 1,
			math.floor(basePos.z + dz * rootLength * 0.5)
		)
		tree_utils.drawBezier(volume, startP, endP, ctrl, rootThickness, 1, math.max(4, rootLength), voxelColor)
	end
end

function tree_utils.createLineRoots(volume, basePos, numRoots, maxLength, thickness, voxelColor, dropY)
	for _ = 1, numRoots do
		local rAngle = math.random() * 2 * math.pi
		local rLen = math.random(2, maxLength)
		local endY = basePos.y
		if dropY then
			endY = basePos.y - math.random(0, 1)
		end
		local rootEnd = g_ivec3.new(
			math.floor(basePos.x + math.cos(rAngle) * rLen),
			endY,
			math.floor(basePos.z + math.sin(rAngle) * rLen)
		)
		g_shape.line(volume, basePos, rootEnd, voxelColor, thickness)
	end
end

function tree_utils.createBaseFlare(volume, basePos, radius, height, voxelColor)
	g_shape.dome(volume, basePos, 'y', false, radius * 2, height, radius * 2, voxelColor)
end

function tree_utils.createCanopyDome(volume, center, width, height, leafColor, leafColor2, topScale, bottomScale)
	topScale = topScale or 1.2
	if bottomScale == nil then bottomScale = 0.7 end
	g_shape.dome(volume, center, 'y', false, width * 2, height, width * 2, leafColor)
	local topCap = g_ivec3.new(center.x, center.y + 1, center.z)
	g_shape.dome(volume, topCap, 'y', false, math.floor(width * topScale), math.max(1, height - 1),
		math.floor(width * topScale), leafColor2)
	if bottomScale > 0 then
		g_shape.dome(volume, center, 'y', true,
			math.floor(width * bottomScale), math.max(1, height - 2), math.floor(width * bottomScale), leafColor)
	end
end

function tree_utils.createSpireTip(volume, topPos, tipHeight, leavesColor, coneWidth)
	coneWidth = coneWidth or 3
	local tipTop = g_ivec3.new(topPos.x, topPos.y + tipHeight, topPos.z)
	g_shape.line(volume, topPos, tipTop, leavesColor, 1)
	g_shape.cone(volume, g_ivec3.new(topPos.x, topPos.y - 1, topPos.z), 'y', false,
		coneWidth, tipHeight + 2, coneWidth, leavesColor)
	return tipTop
end

function tree_utils.dualColorFoliage(volume, center, radius, height, primaryColor, secondaryColor, underFill)
	g_shape.dome(volume, center, 'y', false, radius * 2, height, radius * 2, primaryColor)
	local offX = math.random(-1, 1)
	local offZ = math.random(-1, 1)
	local innerR = math.max(1, radius - 1)
	local innerH = math.max(1, height - 1)
	local sub = g_ivec3.new(center.x + offX, center.y, center.z + offZ)
	g_shape.dome(volume, sub, 'y', false, innerR * 2, innerH, innerR * 2, secondaryColor)
	if underFill then
		local underHeight = math.max(1, math.floor(height / 3))
		g_shape.dome(volume, center, 'y', true, math.floor(radius * 1.4), underHeight,
			math.floor(radius * 1.4), primaryColor)
	end
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
