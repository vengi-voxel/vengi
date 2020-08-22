local module = {}

--[[
function module.translate(x, y, z)
	return bone.new(vec3.new(1.0, 1.0, 1.0), vec3.new(x, y, z), quat.new())
end
--]]

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
