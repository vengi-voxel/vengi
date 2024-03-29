local function addOrGetKeyFrame(node, frame)
	if node:hasKeyFrameForFrame(frame) then
		return node:keyFrameForFrame(frame)
	end
	return node:addKeyFrame(frame)
end

local function animate(node, frame, maxKeyFrames)
	local runTimeFactor = 12.0
	local shoulderScale = 1.1
	local timeFactor = runTimeFactor
	local animTime = frame / 10.0
	local scaledTime = animTime * timeFactor
	local sine = math.sin(scaledTime)
	local cosine = math.cos(scaledTime)
	local movement = 0.35 * sine
	local animTimeCos = math.cos(animTime)
	local headLookX = math.rad(5.0) + 0.05 * animTimeCos
	local headLookY = 0.1 * sine
	local rotateYMovement = g_quat.rotateY(movement)
	local handAngle = 0.2 * sine
	local footAngle = 1.5 * cosine
	if node:name() == "belt" then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(rotateYMovement)
	end

	if node:name() == "foot_right" then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateZ(footAngle))
	end
	if node:name() == "foot_left" then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateZ(-footAngle))
	end

	if node:name() == "head" then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateXY(headLookX, headLookY))
	end

	if node:name() == "shoulder_right" then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalScale(shoulderScale)
	end
	if node:name() == "hand_right" then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateZ(handAngle))
	end

	if node:name() == "shoulder_left" then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalScale(shoulderScale)
	end
	if node:name() == "hand_left" then
		local kf = addOrGetKeyFrame(node, frame)
		kf:setLocalOrientation(g_quat.rotateZ(-handAngle))
	end
end

function main(unused_node, unused_region, unused_color, unused_padding)
	local allNodeIds = g_scenegraph.nodeIds()
	for _, nodeId in ipairs(allNodeIds) do
		local node = g_scenegraph.get(nodeId)
		if node:isModel() or node:isReference() then
			local maxKeyFrames = 3
			for i = 0, maxKeyFrames do
				animate(node, i * 20, maxKeyFrames)
			end
		end
	end
	g_scenegraph.updateTransforms()
end
