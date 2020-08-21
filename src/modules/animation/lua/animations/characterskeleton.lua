local module = {}
local boneutil = require 'animations.boneutil'

module.vec3_one = vec3.new(1.0, 1.0, 1.0)

function module.handBone(skeleton, id, skeletonAttr)
	local hand = skeleton:bone(id)
	hand.scale = module.vec3_one
	return hand
end

function module.footBone(skeleton, id, skeletonAttr)
	local foot = skeleton:bone(id)
	foot.scale = module.vec3_one
	return foot
end

function module.shoulderBone(skeleton, id, skeletonAttr, orientation)
	local shoulder = skeleton:bone(id)
	shoulder.scale = vec3.new(skeletonAttr.shoulderScale)
	shoulder.translation = vec3.new(skeletonAttr.shoulderRight, skeletonAttr.chestHeight, skeletonAttr.shoulderForward)
	shoulder.orientation = orientation
	return shoulder
end

function module.pantsBone(skeleton, skeletonAttr)
	local pants = skeleton:bone("pants")
	pants.scale = module.vec3_one
	return pants
end

function module.chestBone(skeleton, skeletonAttr)
	local pants = skeleton:bone("chest")
	pants.scale = module.vec3_one
	return pants
end

function module.beltBone(skeleton, skeletonAttr)
	local belt = skeleton:bone("belt")
	belt.scale = module.vec3_one
	return belt
end

function module.gliderBone(skeleton, skeletonAttr)
	local torso = skeleton:bone("glider")
	torso.scale = module.vec3_one
	return torso
end

function module.toolBone(skeleton, skeletonAttr, movementY)
	local tool = skeleton:bone("tool")
	tool.scale = vec3.new(skeletonAttr.toolScale)
	tool.translation = vec3.new(skeletonAttr.toolRight, skeletonAttr.headY + 2.0, skeletonAttr.toolForward)
	tool.orientation = boneutil.rotateYZ(math.rad(-90.0) + movementY, math.rad(143.0))
	return tool
end

function module.headBone(skeleton, skeletonAttr)
	local bone = skeleton:bone("head")
	bone.scale = vec3.new(skeletonAttr.headScale)
	return bone
end

return module
