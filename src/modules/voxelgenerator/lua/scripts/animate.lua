--
-- Animate a given scene if the nodes are named correctly.
-- Valid names are belt, head, foot, shoulder or hand with either left or right indicators.
--
-- e.g. belt, belt_left, belt_right, left_belt, right_belt, belt_l, belt_r, l_belt, r_belt
--
-- Model should use a right-handed system - this basically means it should look into the negative
-- z direction (and right shoulder should be along the positive x axis, y is up).
--

function arguments()
	return {
		{ name = 'animation', desc = 'The animation to create', type = 'enum', enum = 'walk,jump', default = 'walk'},
		{ name = 'maxKeyFrames', desc = 'The maximum number of keyframes to create', type = 'int', default = 6, min = 1, max = 100},
		{ name = 'frameDuration', desc = 'How many frames does each key frame last', type = 'int', default = 20, min = 1, max = 1000},
		{ name = 'timeFactor', desc = 'How fast the animation should be', type = 'float', default = 12.0, min = 0.0, max = 100.0},
		{ name = 'handAngleFactor', desc = 'How much the hand should be rotated', type = 'float', default = 0.2, min = 0.0, max = 100.0},
		{ name = 'footAngleFactor', desc = 'How much the foot should be rotated', type = 'float', default = 1.5, min = 0.0, max = 100.0}
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

local function jumpAnimation(node, keyframe, maxKeyFrames, frameDuration, timeFactor)
	local frame = keyframe * frameDuration
	local animTime = keyframe / maxKeyFrames
	local scaledTime = animTime * timeFactor
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

local function walkAnimation(node, keyframe, maxKeyFrames, frameDuration, timeFactor, handAngleFactor, footAngleFactor)
	local frame = keyframe * frameDuration
	local animTime = keyframe / maxKeyFrames
	local scaledTime = animTime * timeFactor
	local sine = math.sin(scaledTime)
	local cosine = math.cos(scaledTime)

	local handAngle = handAngleFactor * sine
	local footAngle = footAngleFactor * cosine

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

function main(_, _, _, animation, maxKeyFrames, frameDuration, timeFactor, handAngleFactor, footAngleFactor)
	if not isValidAnimation(animation) then
		error("Unknown animation: " .. animation)
	end
	local allNodeIds = g_scenegraph.nodeIds()
	for _, nodeId in ipairs(allNodeIds) do
		local node = g_scenegraph.get(nodeId)
		if node:isModel() or node:isReference() then
			for i = 0, maxKeyFrames do
				if animation == "walk" then
					if not walkAnimation(node, i, maxKeyFrames, frameDuration, timeFactor, handAngleFactor, footAngleFactor) then
						break
					end
				elseif animation == "jump" then
					if not jumpAnimation(node, i, maxKeyFrames, frameDuration, timeFactor) then
						break
					end
				end
			end
		end
	end
	g_scenegraph.updateTransforms()
end
