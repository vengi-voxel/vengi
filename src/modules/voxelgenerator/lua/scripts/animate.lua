local function addOrGetKeyFrame(node, frame)
	if node:hasKeyFrameForFrame(frame) then
		return node:keyFrameForFrame(frame)
	end
	return node:addKeyFrame(frame)
end

local function isRight(name, id)
	return name == id .. "right" or name == id .. "r" or name == "right" .. id or name == "r" .. id or name == id .. "_right" or name == id .. "_r" or name == "right_" .. id or name == "r_" .. id
end

local function isLeft(name, id)
	return name == id .. "left" or name == id .. "l" or name == "left" .. id or name == "l" .. id or name == id .. "_left" or name == id .. "_l" or name == "left_" .. id or name == "l_" .. id
end

local function animate(node, keyframe, maxKeyFrames)
	local frame = keyframe * 20
	local runTimeFactor = 12.0
	local shoulderScale = 1.1
	local timeFactor = runTimeFactor
	local animTime = keyframe / maxKeyFrames
	local scaledTime = animTime * timeFactor
	local sine = math.sin(scaledTime)
	local cosine = math.cos(scaledTime)
	local movement = 0.05 * sine
	local animTimeCos = math.cos(animTime)
	local headLookX = math.rad(5.0) + 0.05 * animTimeCos
	local headLookY = 0.1 * sine
	local rotateYMovement = g_quat.rotateY(movement)
	local handAngle = 0.2 * sine
	local footAngle = 1.5 * cosine
	local lowername = string.lower(node:name());

	if lowername == "belt" then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(rotateYMovement)
	elseif lowername == "head" then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateXY(headLookX, headLookY))
	elseif isRight(lowername, "foot") then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateZ(footAngle))
	elseif isLeft(lowername, "foot") then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateZ(-footAngle))
	elseif isRight(lowername, "shoulder") then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalScale(shoulderScale)
	elseif isLeft(lowername, "shoulder") then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalScale(shoulderScale)
	elseif isRight(lowername, "hand") then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateZ(handAngle))
	elseif isLeft(lowername, "hand") then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateZ(-handAngle))
	end
end

function main(_, _, _)
	local allNodeIds = g_scenegraph.nodeIds()
	for _, nodeId in ipairs(allNodeIds) do
		local node = g_scenegraph.get(nodeId)
		if node:isModel() or node:isReference() then
			local maxKeyFrames = 6
			for i = 0, maxKeyFrames do
				animate(node, i, maxKeyFrames)
			end
		end
	end
	g_scenegraph.updateTransforms()
end
