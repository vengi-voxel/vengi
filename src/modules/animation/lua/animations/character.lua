local c_halfPi = math.pi/ 2.0
local toolOrientation = boneutil.rotateYZ(math.rad(-90.0), math.rad(110.0))
local vec3_one = vec3.new(1.0, 1.0, 1.0)

function swim(animTime, velocity, skeleton, skeletonAttr)
	local timeFactor = skeletonAttr.runTimeFactor
	local sin = math.sin
	local cos = math.cos
	local scaledTime = animTime * timeFactor;
	local sine = sin(scaledTime)
	local cosine = cos(scaledTime)
	local cosineSlow = cos(0.25 * scaledTime)
	local movement = 0.15 * sine
	local animTimeCos = cos(animTime)
	local headLookX = math.rad(-30.0) + 0.1 * animTimeCos
	local headLookY = 0.1 * sine
	local pantsY = skeletonAttr.pantsY
	local beltY = skeletonAttr.beltY

	local head = skeleton:headBone()
	head:setScale(skeletonAttr.headScale)
	head:setTranslation(0.0, 0.5 + skeletonAttr.neckHeight + skeletonAttr.headY + 1.3 * cosine, -1.0 + skeletonAttr.neckForward)
	head:setOrientation(boneutil.rotateXY(headLookX, headLookY))

	local rotateYMovement = boneutil.rotateY(movement)
	local bodyMoveY = 0.5 * cosine
	local chest = skeleton:chestBone()
	chest:setScale(1.0)
	chest:setTranslation(0.0, skeletonAttr.chestY + bodyMoveY, 0.0)
	chest:setOrientation(rotateYMovement)

	local belt = skeleton:beltBone()
	belt:setScale(1.0)
	belt:setTranslation(0.0, beltY + bodyMoveY, 0.0)
	belt:setOrientation(rotateYMovement)

	local pants = skeleton:pantsBone()
	pants:setScale(1.0)
	pants:setTranslation(0.0, pantsY + bodyMoveY, 0.0)
	pants:setOrientation(rotateYMovement)

	local handAngle = 0.05 * sine
	local handMoveY = 3.0 * cosineSlow
	local handMoveX = math.abs(4.0 * cosineSlow)
	local righthand = skeleton:righthandBone()
	righthand:setTranslation(0.1 + skeletonAttr.handRight + handMoveX, handMoveY, skeletonAttr.handForward)
	righthand:setOrientation(boneutil.rotateX(handAngle))

	local lefthand = skeleton:lefthandBone()
	lefthand:mirrorXZ(righthand)
	lefthand:setOrientation(boneutil.rotateX(-handAngle))

	local footAngle = 0.8 * cosine
	local footMoveY = 0.001 * cosine
	local rightfoot = skeleton:rightfootBone()
	rightfoot:setTranslation(skeletonAttr.footRight, skeletonAttr.hipOffset - footMoveY, 0.0)
	rightfoot:setOrientation(boneutil.rotateX(footAngle))

	skeleton:leftfootBone():mirrorX(rightfoot)

	local tool = skeleton:toolBone()
	tool:setScale(0.8 * skeletonAttr.toolScale)
	tool:setTranslation(skeletonAttr.toolRight, pantsY, skeletonAttr.toolForward)
	tool:setOrientation(toolOrientation)

	local rightshoulder = skeleton:rightshoulderBone()
	rightshoulder:setScale(skeletonAttr.shoulderScale)
	rightshoulder:setTranslation(skeletonAttr.shoulderRight, skeletonAttr.chestHeight, skeletonAttr.shoulderForward)
	rightshoulder:setOrientation(boneutil.rotateX(movement))

	local torsoScale = skeletonAttr.scaler
	local torso = skeleton:torsoBone(torsoScale)
	torso:setTranslation(0.0, 0.5 + 0.04 * sine, -beltY * torsoScale)
	torso:setOrientation(boneutil.rotateXZ(-0.2 + c_halfPi + 0.15 * cosine, 0.1 * cosine))

	skeleton:hideGliderBone()

	skeleton:leftshoulderBone():mirrorX(rightshoulder)
end
