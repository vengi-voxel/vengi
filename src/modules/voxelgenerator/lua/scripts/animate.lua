--
-- Animate a given scene if the nodes are named correctly.
-- Valid names are belt, head, foot, shoulder or hand with either left or right indicators.
--
-- e.g. belt, belt_left, belt_right, left_belt, right_belt, belt_l, belt_r, l_belt, r_belt
--
-- Model should use a right-handed system - this basically means it should look into the negative
-- z direction (and right shoulder should be along the positive x axis, y is up).
--
-- The model should have the correct parent and child relationships (a hand or arm is a child of a
-- shoulder, a foot is child of a leg, etc).
--
-- When adding new animations, check the NEW_ANIM comments and follow the instructions
--

-- TODO: add tool or other attachment (via point nodes) support for this animation script
-- TODO: use torso and make hierarchie clearer

function arguments()
	return {
		-- NEW_ANIM: add new animation ids to the enum values
		-- if values are animation specific, please mark them as such by mentioning the animation name like this "(walk)"
		{ name = 'animation', desc = 'The animation to create', type = 'enum', enum = 'walk,jump,all', default = 'walk'},
		{ name = 'createAnim', desc = 'Create the animation by duplicating the current animation or add to current animation', type = 'bool', default = 'true' },
		{ name = 'maxKeyFrames', desc = 'The maximum number of keyframes to create', type = 'int', default = 6, min = 1, max = 100},
		{ name = 'frameDuration', desc = 'How many frames does each key frame last', type = 'int', default = 20, min = 1, max = 1000},
		{ name = 'timeFactor', desc = 'How fast the animation should be', type = 'float', default = 12.0, min = 0.0, max = 100.0},
		{ name = 'handAngleFactor', desc = 'How much the hand should be rotated (walk)', type = 'float', default = 0.2, min = 0.0, max = 100.0},
		{ name = 'footAngleFactor', desc = 'How much the foot should be rotated (walk)', type = 'float', default = 1.5, min = 0.0, max = 100.0}
	}
end

local function addOrGetKeyFrame(node, frame)
	if node:hasKeyFrameForFrame(frame) then
		return node:keyFrameForFrame(frame)
	end
	return node:addKeyFrame(frame)
end

local function isRight(name, id)
	return name == id .. "right" or name == id .. "r" or name == "right" .. id or name == "r" .. id or
		name == id .. "_right" or
		name == id .. "_r" or
		name == "right_" .. id or
		name == "r_" .. id
end

local function isLeft(name, id)
	return name == id .. "left" or name == id .. "l" or name == "left" .. id or name == "l" .. id or name == id .. "_left" or
		name == id .. "_l" or
		name == "left_" .. id or
		name == "l_" .. id
end

local function jumpAnimation(node, keyframe, context)
	local frame = keyframe * context.frameDuration
	local animTime = keyframe / context.maxKeyFrames
	local scaledTime = animTime * context.timeFactor
	local sine = math.sin(scaledTime)
	local sineSlow = math.sin(scaledTime / 2.0)
	local sineStop = math.sin(math.min(animTime * 5.0, math.pi / 2))
	local sineStopAlt = math.sin(math.min(animTime * 4.5, math.pi / 2))
	local handWaveStop = sineStopAlt * 0.6

	local lowername = string.lower(node:name())
	if lowername == "head" then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateX(0.25 + sineStop * 0.1 + sineSlow * 0.04))
	elseif isRight(lowername, "hand") then
		local kf = addOrGetKeyFrame(node, frame)
		local sineHand = sine * 0.4
		local sineStopHandY = sineStop * 3.2 - sineHand
		local sineStopHandZ = sineStop * 3.8
		kf:setLocalTranslation(0.5, sineStopHandY, sineStopHandZ)
		kf:setLocalOrientation(g_quat.rotateX(-handWaveStop))
	elseif isLeft(lowername, "hand") then
		local kf = addOrGetKeyFrame(node, frame)
		local sineHand = sine * 0.4
		local sineStopHandY = sineStop * 3.2 - sineHand
		local sineStopHandZ = sineStop * 3.8
		kf:setLocalTranslation(-0.5, sineStopHandY, -sineStopHandZ)
		kf:setLocalOrientation(g_quat.rotateX(handWaveStop))
	elseif isRight(lowername, "foot") then
		local kf = addOrGetKeyFrame(node, frame)
		local sineStopFoot = sineStop * 1.2
		local sineSlowFoot = sineSlow * 0.2
		kf:setLocalOrientation(g_quat.rotateX(-sineStopFoot + sineSlowFoot))
	elseif isLeft(lowername, "foot") then
		local kf = addOrGetKeyFrame(node, frame)
		local sineStopFoot = sineStop * 1.2
		local sineSlowFoot = sineSlow * 0.2
		kf:setLocalOrientation(g_quat.rotateX(sineStopFoot + sineSlowFoot))
	elseif isRight(lowername, "shoulder") then
		local kf = addOrGetKeyFrame(node, frame)
		local sineStopShoulder = sineStopAlt * 0.3
		kf:setLocalOrientation(g_quat.rotateX(-sineStopShoulder))
	elseif isLeft(lowername, "shoulder") then
		local kf = addOrGetKeyFrame(node, frame)
		local sineStopShoulder = sineStopAlt * 0.3
		kf:setLocalOrientation(g_quat.rotateX(sineStopShoulder))
	else
		g_log.info(
			"No animation for " ..
				node:name() .. ". Name them belt, head, foot, shoulder or hand with left and right indicators."
		)
		return false
	end
	return true
end

local function walkAnimation(node, keyframe, context)
	local frame = keyframe * context.frameDuration
	local animTime = keyframe / context.maxKeyFrames
	local scaledTime = animTime * context.timeFactor
	local sine = math.sin(scaledTime)
	local cosine = math.cos(scaledTime)

	local handAngle = context.handAngleFactor * sine
	local footAngle = context.footAngleFactor * cosine

	local lowername = string.lower(node:name())
	if lowername == "belt" then
		local movement = 0.05 * sine
		local rotateYMovement = g_quat.rotateY(movement)
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(rotateYMovement)
	elseif lowername == "head" then
		local animTimeCos = math.cos(animTime)
		local headLookX = 0.05 * animTimeCos
		local headLookY = 0.1 * sine
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateXY(headLookX, headLookY))
	elseif isRight(lowername, "foot") then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateX(footAngle))
	elseif isLeft(lowername, "foot") then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateX(-footAngle))
	elseif isRight(lowername, "hand") then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateX(handAngle))
	elseif isLeft(lowername, "hand") then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateX(-handAngle))
	else
		g_log.info(
			"No animation for " ..
				node:name() .. ". Name them belt, head, foot, shoulder or hand with left and right indicators."
		)
		return false
	end
	return true
end

-- A wrapper around all the animation functions
-- NEW_ANIM: When adding new animations, don't forget to register your animation function here
local function allAnimation(node, context)
	walkAnimation(node, context)
	jumpAnimation(node, context)
end

-- Check if the animation is valid by checking if it is in the enum if the arguments
local function isValidAnimation(animation)
	local args = arguments()
	for _, arg in ipairs(args) do
		if arg.name == "animation" then
			local pattern = "%f[%w]" .. animation .. "%f[%W]"
			if arg.enum:match(pattern) then
				return true
			end
		end
	end
	return false
end

local function createAnimation(node, context)
	-- NEW_ANIM: When adding new animations, don't forget to register your animation function here
	local animationMapping = {
		walk = walkAnimation,
		jump = jumpAnimation,
		all = allAnimation,
	}
	local animFunc = animationMapping[context.animation]
	if animFunc == nil then
		error("No animation callback registered for: " .. context.animation)
	end
	if context.createAnim then
		g_scenegraph.duplicateAnimation(g_scenegraph.activeAnimation(), context.animation)
		g_scenegraph.setAnimation(context.animation)
	end
	for keyframe = 0, context.maxKeyFrames - 1 do
		if not animFunc(node, keyframe, context) then
			break
		end
	end
end

function main(_, _, _, animation, createAnim, maxKeyFrames, frameDuration, timeFactor, handAngleFactor, footAngleFactor)
	if not isValidAnimation(animation) then
		error("Unknown animation: " .. animation)
	end
	-- NEW_ANIM: Add new context variables here if your animation needs them
	context = {
		createAnim = createAnim,
		animation = animation,
		maxKeyFrames = maxKeyFrames,
		frameDuration = frameDuration,
		timeFactor = timeFactor,
		handAngleFactor = handAngleFactor,
		footAngleFactor = footAngleFactor,
	}

	local allNodeIds = g_scenegraph.nodeIds()
	for _, nodeId in ipairs(allNodeIds) do
		local node = g_scenegraph.get(nodeId)
		if node:isModel() or node:isReference() or node:isPoint() then
			createAnimation(node, context)
		end
	end
	g_scenegraph.updateTransforms()
end
