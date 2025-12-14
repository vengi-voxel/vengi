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
		g_shape.cube(volume, p, width, thickness, thickness, voxel)
		p.x = p.x + width
		g_shape.cube(volume, p, thickness, height, thickness, voxel)
		p.x = p.x + math.floor(thickness / 2)
		p.z = p.z + math.floor(thickness / 2)
	elseif depth ~= 0 then
		g_shape.cube(volume, p, thickness, thickness, depth, voxel)
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

return tree_utils
