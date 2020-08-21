local module = {}

--[[
function module.translate(x, y, z)
	return bone.new(vec3.new(1.0, 1.0, 1.0), vec3.new(x, y, z), quat.new())
end
--]]

function module.angleAxis(angle, direction)
	local s = math.sin(angle * 0.5)
	local sv = vec3.new(s, s, s)
	local v = direction * sv
	return quat.new(math.cos(angle * 0.5), v.x, v.y, v.z)
end

module.vec3_backward = vec3.new(0.0, 0.0, 1.0)
module.vec3_right    = vec3.new(1.0, 0.0, 0.0)
module.vec3_up       = vec3.new(0.0, 1.0, 0.0)

function module.rotateX(angle)
	return module.angleAxis(angle, module.vec3_right)
end

function module.rotateZ(angle)
	return module.angleAxis(angle, module.vec3_backward)
end

function module.rotateY(angle)
	return module.angleAxis(angle, module.vec3_up)
end

function module.rotateXZ(angleX, angleZ)
	return module.rotateX(angleX) * module.rotateZ(angleZ)
end

function module.rotateYZ(angleY, angleZ)
	return module.rotateZ(angleZ) * module.rotateY(angleY)
end

function module.rotateXY(angleX, angleY)
	return module.rotateY(angleY) * module.rotateX(angleX)
end

function module.rotateXYZ(angleX, angleY, angleZ)
	return module.rotateXY(angleX, angleY) * module.rotateZ(angleZ)
end

function module.mirrorX(bone)
	local mirrored = bone
	mirrored.translation.x = mirrored.translation.x * -1.0
	-- the winding order is fixed by reverse index buffer filling
	mirrored.scale.x = mirrored.scale.x * -1.0
	return mirrored
end

function module.mirrorXYZ(bone)
	local mirrored = bone
	mirrored.translation = mirrored.translation * -1.0
	-- the winding order is fixed by reverse index buffer filling
	mirrored.scale = mirrored.scale * -1.0
	return mirrored
end

function module.boneMirrorXZ(bone)
	local mirrored = bone
	mirrored.translation.x = mirrored.translation.x * -1.0
	mirrored.translation.z = mirrored.translation.z * -1.0
	-- the winding order is fixed by reverse index buffer filling
	mirrored.scale.x = mirrored.scale.x * -1.0
	mirrored.scale.z = mirrored.scale.z * -1.0
	return mirrored
end

function module.mirrorXZ(translation)
	translation.x = translation.x * -1.0
	translation.z = translation.z * -1.0
	return translation
end

function module.clamp(v, min, max)
	if v <= min then
		return min
	end
	if v >= max then
		return max
	end
	return v
end

return module
