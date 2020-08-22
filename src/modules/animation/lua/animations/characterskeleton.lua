local module = {}
local boneutil = require 'animations.boneutil'

module.vec3_one = vec3.new(1.0, 1.0, 1.0)

function module.shoulderBone(skeleton, id, skeletonAttr, orientation)
	local shoulder = skeleton:bone(id)
	shoulder.scale = vec3.new(skeletonAttr.shoulderScale)
	shoulder.translation = vec3.new(skeletonAttr.shoulderRight, skeletonAttr.chestHeight, skeletonAttr.shoulderForward)
	shoulder.orientation = orientation
	return shoulder
end

function module.toolBone(skeleton, skeletonAttr, movementY)
	local tool = skeleton:bone("tool")
	tool.scale = vec3.new(skeletonAttr.toolScale)
	tool.translation = vec3.new(skeletonAttr.toolRight, skeletonAttr.headY + 2.0, skeletonAttr.toolForward)
	tool.orientation = boneutil.rotateYZ(math.rad(-90.0) + movementY, math.rad(143.0))
	return tool
end

return module
