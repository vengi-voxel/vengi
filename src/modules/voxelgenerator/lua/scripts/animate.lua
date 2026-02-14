--
-- Animate a given scene if the nodes are named correctly.
-- Node names must contain a body part keyword and optionally a side indicator.
-- Body part keywords: belt, torso, waist, body, chest, spine, core, head,
--   foot, toe, hand, arm, shoulder, leg, thigh, hip, knee, cover, cape, cloak.
-- Side indicators: left/right or _l/_r prefix/suffix.
-- Segment indicators: _u (upper) / _l (lower) for legs (when side is already
--   determined by left/right keyword).
--
-- e.g. belt, arm_left, K_Arm_Left, right_leg, K_Leg_Right_u, shoulder_r
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

function arguments()
	return {
		-- NEW_ANIM: add new animation ids to the enum values
		{ name = 'animation', desc = 'The animation to create', type = 'enum', enum = 'walk,run,idle,wave,jump,tpose,all', default = 'walk'},
		{ name = 'createAnim', desc = 'Create the animation by duplicating the current animation or add to current animation', type = 'bool', default = 'true' },
		{ name = 'maxKeyFrames', desc = 'The maximum number of keyframes to create', type = 'int', default = 12, min = 1, max = 100},
		{ name = 'frameDuration', desc = 'How many frames does each key frame last', type = 'int', default = 10, min = 1, max = 1000},
		{ name = 'handAngleFactor', desc = 'How much the hand/arm should be rotated', type = 'float', default = 0.4, min = 0.0, max = 3.0},
		{ name = 'footAngleFactor', desc = 'How much the foot/leg should be rotated', type = 'float', default = 0.5, min = 0.0, max = 3.0}
	}
end

function description()
	return "Animate a given scene if the nodes are named correctly. "
		.. "Node names must contain a body part keyword (belt/torso/waist/core/head/foot/toe/"
		.. "hand/arm/shoulder/leg/thigh/hip/knee/cover/cape) with optional left/right indicators. "
		.. "Supports upper/lower leg segments (e.g. K_Leg_Left_u, K_Leg_Left_l). "
		.. "Supports walk, run, idle, wave, jump and t-pose."
end

---------------------------------------------------------------------------
-- Helper utilities
---------------------------------------------------------------------------

local function addOrGetKeyFrame(node, frame)
	if node:hasKeyFrameForFrame(frame) then
		return node:keyFrameForFrame(frame)
	end
	return node:addKeyFrame(frame)
end

---------------------------------------------------------------------------
-- Body part identification (shared module)
---------------------------------------------------------------------------

local bodypart = require "modules.bodypart"

-- Convenience alias
local function identifyBodyPart(name)
	return bodypart.identify(name)
end

---------------------------------------------------------------------------
-- Walk animation
--
-- Natural walk cycle: opposite arm/leg pairs swing together.
-- Right foot forward = left arm forward and vice-versa.
-- Full cycle over 2*pi so frame 0 == frame N for seamless looping.
---------------------------------------------------------------------------
local function walkAnimation(node, keyframe, context)
	local frame = keyframe * context.frameDuration
	local phase = (keyframe / context.maxKeyFrames) * math.pi * 2.0

	local sinPhase = math.sin(phase)
	local cosPhase = math.cos(phase)

	local footSwing = context.footAngleFactor * sinPhase
	local armSwing  = context.handAngleFactor * sinPhase

	local part = identifyBodyPart(node:name())
	if part == nil then
		g_log.debug("walk: no animation for " .. node:name())
		return false
	end

	local kf = addOrGetKeyFrame(node, frame)

	if part == "torso" or part == "cover" then
		-- subtle sway and vertical bob
		local sway = 0.04 * sinPhase
		local bob  = 0.015 * math.abs(math.sin(phase * 2.0))
		kf:setLocalOrientation(g_quat.rotateY(sway))
		kf:setLocalTranslation(0, bob, 0)

	elseif part == "head" then
		-- very subtle head bob, counter the torso sway slightly
		local headBob = 0.02 * math.sin(phase * 2.0)
		local headSway = -0.02 * sinPhase
		kf:setLocalOrientation(g_quat.rotateXY(headBob, headSway))

	elseif part == "right_upper_leg" then
		kf:setLocalOrientation(g_quat.rotateX(footSwing))

	elseif part == "left_upper_leg" then
		kf:setLocalOrientation(g_quat.rotateX(-footSwing))

	elseif part == "right_lower_leg" then
		-- lower leg: reduced swing
		kf:setLocalOrientation(g_quat.rotateX(footSwing * 0.6))

	elseif part == "left_lower_leg" then
		kf:setLocalOrientation(g_quat.rotateX(-footSwing * 0.6))

	elseif part == "right_knee" then
		kf:setLocalOrientation(g_quat.rotateX(footSwing * 0.3))

	elseif part == "left_knee" then
		kf:setLocalOrientation(g_quat.rotateX(-footSwing * 0.3))

	elseif part == "right_foot" then
		kf:setLocalOrientation(g_quat.rotateX(footSwing * 0.4))

	elseif part == "left_foot" then
		kf:setLocalOrientation(g_quat.rotateX(-footSwing * 0.4))

	elseif part == "right_toe" then
		kf:setLocalOrientation(g_quat.rotateX(footSwing * 0.2))

	elseif part == "left_toe" then
		kf:setLocalOrientation(g_quat.rotateX(-footSwing * 0.2))

	elseif part == "right_shoulder" or part == "right_arm" or part == "right_hand" then
		-- right arm swings opposite to right leg
		kf:setLocalOrientation(g_quat.rotateX(-armSwing))

	elseif part == "left_shoulder" or part == "left_arm" or part == "left_hand" then
		kf:setLocalOrientation(g_quat.rotateX(armSwing))
	end

	return true
end

---------------------------------------------------------------------------
-- Run animation
--
-- Similar to walk but larger amplitudes, more forward lean, more bounce.
---------------------------------------------------------------------------
local function runAnimation(node, keyframe, context)
	local frame = keyframe * context.frameDuration
	local phase = (keyframe / context.maxKeyFrames) * math.pi * 2.0

	local sinPhase = math.sin(phase)
	local cosPhase = math.cos(phase)

	local footSwing = context.footAngleFactor * 1.8 * sinPhase
	local armSwing  = context.handAngleFactor * 1.6 * sinPhase

	local part = identifyBodyPart(node:name())
	if part == nil then
		g_log.debug("run: no animation for " .. node:name())
		return false
	end

	local kf = addOrGetKeyFrame(node, frame)

	if part == "torso" or part == "cover" then
		-- forward lean + bounce
		local lean = 0.15
		local sway = 0.06 * sinPhase
		local bounce = 0.04 * math.abs(math.sin(phase * 2.0))
		kf:setLocalOrientation(g_quat.rotateXY(lean, sway))
		kf:setLocalTranslation(0, bounce, 0)

	elseif part == "head" then
		-- counter the forward lean, slight bob
		local counterLean = -0.1
		local headBob = 0.03 * math.sin(phase * 2.0)
		kf:setLocalOrientation(g_quat.rotateX(counterLean + headBob))

	elseif part == "right_upper_leg" then
		-- more knee bend at the back of the stride
		local bendBack = math.max(0, -sinPhase) * 0.3
		kf:setLocalOrientation(g_quat.rotateX(footSwing + bendBack))

	elseif part == "left_upper_leg" then
		local bendBack = math.max(0, sinPhase) * 0.3
		kf:setLocalOrientation(g_quat.rotateX(-footSwing + bendBack))

	elseif part == "right_lower_leg" then
		-- lower leg: reduced swing with knee bend
		local bendBack = math.max(0, -sinPhase) * 0.2
		kf:setLocalOrientation(g_quat.rotateX(footSwing * 0.6 + bendBack))

	elseif part == "left_lower_leg" then
		local bendBack = math.max(0, sinPhase) * 0.2
		kf:setLocalOrientation(g_quat.rotateX(-footSwing * 0.6 + bendBack))

	elseif part == "right_knee" then
		kf:setLocalOrientation(g_quat.rotateX(footSwing * 0.3))

	elseif part == "left_knee" then
		kf:setLocalOrientation(g_quat.rotateX(-footSwing * 0.3))

	elseif part == "right_foot" then
		local bendBack = math.max(0, -sinPhase) * 0.15
		kf:setLocalOrientation(g_quat.rotateX(footSwing * 0.4 + bendBack))

	elseif part == "left_foot" then
		local bendBack = math.max(0, sinPhase) * 0.15
		kf:setLocalOrientation(g_quat.rotateX(-footSwing * 0.4 + bendBack))

	elseif part == "right_toe" then
		kf:setLocalOrientation(g_quat.rotateX(footSwing * 0.2))

	elseif part == "left_toe" then
		kf:setLocalOrientation(g_quat.rotateX(-footSwing * 0.2))

	elseif part == "right_shoulder" or part == "right_arm" or part == "right_hand" then
		-- arms pump with some elbow bend via Z
		local elbowBend = 0.2 + 0.15 * math.max(0, sinPhase)
		kf:setLocalOrientation(g_quat.rotateX(-armSwing) * g_quat.rotateZ(-elbowBend))

	elseif part == "left_shoulder" or part == "left_arm" or part == "left_hand" then
		local elbowBend = 0.2 + 0.15 * math.max(0, -sinPhase)
		kf:setLocalOrientation(g_quat.rotateX(armSwing) * g_quat.rotateZ(elbowBend))
	end

	return true
end

---------------------------------------------------------------------------
-- Idle / breathing animation
--
-- Subtle breathing motion: torso rises/falls, arms sway gently.
---------------------------------------------------------------------------
local function idleAnimation(node, keyframe, context)
	local frame = keyframe * context.frameDuration
	local phase = (keyframe / context.maxKeyFrames) * math.pi * 2.0

	local sinPhase = math.sin(phase)

	local part = identifyBodyPart(node:name())
	if part == nil then
		g_log.debug("idle: no animation for " .. node:name())
		return false
	end

	local kf = addOrGetKeyFrame(node, frame)

	if part == "torso" or part == "cover" then
		-- breathing: slight rise and fall
		local breathe = 0.012 * sinPhase
		kf:setLocalTranslation(0, breathe, 0)
		kf:setLocalOrientation(g_quat.new())

	elseif part == "head" then
		-- very subtle look around
		local lookY = 0.03 * math.sin(phase * 0.5)
		kf:setLocalOrientation(g_quat.rotateY(lookY))

	elseif part == "right_shoulder" or part == "right_arm" or part == "right_hand" then
		-- arms hang naturally, slight sway
		local sway = 0.02 * sinPhase
		kf:setLocalOrientation(g_quat.rotateZ(-sway))

	elseif part == "left_shoulder" or part == "left_arm" or part == "left_hand" then
		local sway = 0.02 * sinPhase
		kf:setLocalOrientation(g_quat.rotateZ(sway))

	elseif part == "right_upper_leg" or part == "right_lower_leg" or part == "right_knee"
		or part == "right_foot" or part == "right_toe"
		or part == "left_upper_leg" or part == "left_lower_leg" or part == "left_knee"
		or part == "left_foot" or part == "left_toe" then
		-- legs stay still during idle
		kf:setLocalOrientation(g_quat.new())
	end

	return true
end

---------------------------------------------------------------------------
-- Wave animation
--
-- Right hand waves while the rest of the body stays mostly still.
---------------------------------------------------------------------------
local function waveAnimation(node, keyframe, context)
	local frame = keyframe * context.frameDuration
	local phase = (keyframe / context.maxKeyFrames) * math.pi * 2.0

	local sinPhase = math.sin(phase)

	local part = identifyBodyPart(node:name())
	if part == nil then
		g_log.debug("wave: no animation for " .. node:name())
		return false
	end

	local kf = addOrGetKeyFrame(node, frame)

	if part == "right_shoulder" or part == "right_arm" then
		-- raise right arm forward and up, then wave side to side
		local raise = 2.0  -- arm forward and above horizontal (~115 degrees)
		local wave = 0.4 * sinPhase
		kf:setLocalOrientation(g_quat.rotateX(raise) * g_quat.rotateZ(wave))

	elseif part == "right_hand" then
		-- wrist wave back and forth
		local wristWave = 0.5 * math.sin(phase * 2.0)
		kf:setLocalOrientation(g_quat.rotateZ(wristWave))

	elseif part == "torso" or part == "cover" then
		-- slight lean to compensate for raised arm
		local lean = 0.04 * sinPhase
		kf:setLocalOrientation(g_quat.rotateZ(lean))

	elseif part == "head" then
		-- look slightly toward the waving hand direction
		local look = 0.1 * sinPhase
		kf:setLocalOrientation(g_quat.rotateY(look))

	else
		-- everything else stays neutral
		kf:setLocalOrientation(g_quat.new())
	end

	return true
end

---------------------------------------------------------------------------
-- Jump animation
--
-- Anticipation → launch → air → landing sequence.
-- Uses normalized time (0-1) to drive different phases.
---------------------------------------------------------------------------
local function jumpAnimation(node, keyframe, context)
	local frame = keyframe * context.frameDuration
	-- normalized time [0, 1)
	local t = keyframe / context.maxKeyFrames

	-- Phase breakdown:
	-- 0.0 - 0.2: anticipation (crouch down)
	-- 0.2 - 0.4: launch (extend upward)
	-- 0.4 - 0.7: airborne (arms/legs spread)
	-- 0.7 - 1.0: landing (absorb impact)

	local part = identifyBodyPart(node:name())
	if part == nil then
		g_log.debug("jump: no animation for " .. node:name())
		return false
	end

	local kf = addOrGetKeyFrame(node, frame)

	-- smooth step helper
	local function smoothstep(edge0, edge1, x)
		local v = math.max(0, math.min(1, (x - edge0) / (edge1 - edge0)))
		return v * v * (3 - 2 * v)
	end

	if part == "torso" or part == "cover" then
		local crouch = 0
		local leanFwd = 0
		if t < 0.2 then
			-- anticipation: crouch down
			crouch = -smoothstep(0, 0.2, t) * 0.5
			leanFwd = smoothstep(0, 0.2, t) * 0.2
		elseif t < 0.4 then
			-- launch: extend upward
			crouch = -0.5 + smoothstep(0.2, 0.4, t) * 1.5
			leanFwd = 0.2 - smoothstep(0.2, 0.4, t) * 0.2
		elseif t < 0.7 then
			-- airborne
			crouch = 1.0 - smoothstep(0.4, 0.7, t) * 0.3
			leanFwd = 0
		else
			-- landing
			crouch = 0.7 - smoothstep(0.7, 1.0, t) * 1.0
			leanFwd = smoothstep(0.7, 0.9, t) * 0.15 - smoothstep(0.9, 1.0, t) * 0.15
		end
		kf:setLocalTranslation(0, crouch, 0)
		kf:setLocalOrientation(g_quat.rotateX(leanFwd))

	elseif part == "head" then
		-- counter the torso lean to keep head somewhat level
		local counterLean = 0
		if t < 0.2 then
			counterLean = -smoothstep(0, 0.2, t) * 0.15
		elseif t < 0.4 then
			counterLean = -0.15 + smoothstep(0.2, 0.4, t) * 0.15
		end
		kf:setLocalOrientation(g_quat.rotateX(counterLean))

	elseif part == "right_upper_leg" or part == "right_foot" then
		local bend = 0
		if t < 0.2 then
			bend = smoothstep(0, 0.2, t) * 0.6  -- deep bend
		elseif t < 0.4 then
			bend = 0.6 - smoothstep(0.2, 0.4, t) * 0.9  -- extend
		elseif t < 0.7 then
			bend = -0.3 + smoothstep(0.4, 0.7, t) * 0.1
		else
			bend = -0.2 + smoothstep(0.7, 1.0, t) * 0.5
			bend = bend - smoothstep(0.85, 1.0, t) * 0.3
		end
		kf:setLocalOrientation(g_quat.rotateX(bend))

	elseif part == "left_upper_leg" or part == "left_foot" then
		local bend = 0
		if t < 0.2 then
			bend = smoothstep(0, 0.2, t) * 0.6
		elseif t < 0.4 then
			bend = 0.6 - smoothstep(0.2, 0.4, t) * 0.9
		elseif t < 0.7 then
			bend = -0.3 + smoothstep(0.4, 0.7, t) * 0.1
		else
			bend = -0.2 + smoothstep(0.7, 1.0, t) * 0.5
			bend = bend - smoothstep(0.85, 1.0, t) * 0.3
		end
		kf:setLocalOrientation(g_quat.rotateX(bend))

	elseif part == "right_lower_leg" or part == "right_knee" or part == "right_toe" then
		-- lower leg / knee / toe: reduced bend
		local bend = 0
		if t < 0.2 then
			bend = smoothstep(0, 0.2, t) * 0.4
		elseif t < 0.4 then
			bend = 0.4 - smoothstep(0.2, 0.4, t) * 0.6
		elseif t < 0.7 then
			bend = -0.2 + smoothstep(0.4, 0.7, t) * 0.1
		else
			bend = -0.1 + smoothstep(0.7, 1.0, t) * 0.3
			bend = bend - smoothstep(0.85, 1.0, t) * 0.2
		end
		kf:setLocalOrientation(g_quat.rotateX(bend))

	elseif part == "left_lower_leg" or part == "left_knee" or part == "left_toe" then
		local bend = 0
		if t < 0.2 then
			bend = smoothstep(0, 0.2, t) * 0.4
		elseif t < 0.4 then
			bend = 0.4 - smoothstep(0.2, 0.4, t) * 0.6
		elseif t < 0.7 then
			bend = -0.2 + smoothstep(0.4, 0.7, t) * 0.1
		else
			bend = -0.1 + smoothstep(0.7, 1.0, t) * 0.3
			bend = bend - smoothstep(0.85, 1.0, t) * 0.2
		end
		kf:setLocalOrientation(g_quat.rotateX(bend))

	elseif part == "right_shoulder" or part == "right_arm" or part == "right_hand" then
		local raise = 0
		if t < 0.2 then
			raise = smoothstep(0, 0.2, t) * 0.3
		elseif t < 0.4 then
			raise = 0.3 - smoothstep(0.2, 0.4, t) * 1.2
		elseif t < 0.7 then
			raise = -0.9
		else
			raise = -0.9 + smoothstep(0.7, 1.0, t) * 0.9
		end
		kf:setLocalOrientation(g_quat.rotateX(raise))

	elseif part == "left_shoulder" or part == "left_arm" or part == "left_hand" then
		local raise = 0
		if t < 0.2 then
			raise = smoothstep(0, 0.2, t) * 0.3
		elseif t < 0.4 then
			raise = 0.3 - smoothstep(0.2, 0.4, t) * 1.2
		elseif t < 0.7 then
			raise = -0.9
		else
			raise = -0.9 + smoothstep(0.7, 1.0, t) * 0.9
		end
		kf:setLocalOrientation(g_quat.rotateX(raise))
	end

	return true
end

---------------------------------------------------------------------------
-- T-Pose
--
-- Resets all parts to a standard T-pose. Only creates a single keyframe.
-- Arms out to the sides, everything else neutral.
---------------------------------------------------------------------------
local function tposeAnimation(node, keyframe, context)
	-- T-pose only needs frame 0
	if keyframe > 0 then
		return true
	end

	local frame = 0

	local part = identifyBodyPart(node:name())
	if part == nil then
		g_log.debug("tpose: no animation for " .. node:name())
		return false
	end

	local kf = addOrGetKeyFrame(node, frame)

	if part == "right_shoulder" or part == "right_arm" then
		-- arm straight out to the right: rotate -90 degrees around Z
		kf:setLocalOrientation(g_quat.rotateZ(-math.pi / 2.0))
		kf:setLocalTranslation(0, 0, 0)
	elseif part == "left_shoulder" or part == "left_arm" then
		-- arm straight out to the left: rotate +90 degrees around Z
		kf:setLocalOrientation(g_quat.rotateZ(math.pi / 2.0))
		kf:setLocalTranslation(0, 0, 0)
	else
		-- everything else: identity (neutral pose)
		kf:setLocalOrientation(g_quat.new())
		kf:setLocalTranslation(0, 0, 0)
	end

	return true
end

---------------------------------------------------------------------------
-- All animation: apply walk, run, idle, wave, jump as separate animations.
---------------------------------------------------------------------------
local animationMapping = {
	walk = walkAnimation,
	run = runAnimation,
	idle = idleAnimation,
	wave = waveAnimation,
	jump = jumpAnimation,
	tpose = tposeAnimation,
}

local function allAnimation(node, keyframe, context)
	-- not meaningful per-keyframe; handled specially in createAnimations
	return true
end

-- Check if the animation is valid by checking if it is in the enum of the arguments
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

-- Remove all keyframes except the first one for a given node
local function clearKeyFrames(node)
	local count = node:numKeyFrames()
	for i = count - 1, 1, -1 do
		node:removeKeyFrame(i)
	end
end

local function createAnimation(nodes, context, animName, animFunc)
	if context.createAnim then
		if not g_scenegraph.hasAnimation(animName) then
			g_scenegraph.duplicateAnimation(g_scenegraph.activeAnimation(), animName)
		end
		g_scenegraph.setAnimation(animName)
		-- Clear pre-existing keyframes so the animation is rebuilt from scratch
		for _, node in ipairs(nodes) do
			clearKeyFrames(node)
		end
	end
	local maxKF = context.maxKeyFrames
	-- t-pose only needs 1 keyframe
	if animName == "tpose" then
		maxKF = 1
	end
	for _, node in ipairs(nodes) do
		for keyframe = 0, maxKF - 1 do
			if not animFunc(node, keyframe, context) then
				break
			end
		end
	end
end

function main(_, _, _, animation, createAnim, maxKeyFrames, frameDuration, handAngleFactor, footAngleFactor)
	if not isValidAnimation(animation) then
		error("Unknown animation: " .. animation)
	end

	local context = {
		createAnim = createAnim,
		animation = animation,
		maxKeyFrames = maxKeyFrames,
		frameDuration = frameDuration,
		handAngleFactor = handAngleFactor,
		footAngleFactor = footAngleFactor,
	}

	-- Collect all nodes that can be animated
	local animatableNodes = {}
	local allNodeIds = g_scenegraph.nodeIds()
	for _, nodeId in ipairs(allNodeIds) do
		local node = g_scenegraph.get(nodeId)
		if node:isModel() or node:isReference() or node:isPoint() then
			local part = identifyBodyPart(node:name())
			if part ~= nil then
				animatableNodes[#animatableNodes + 1] = node
			else
				g_log.info(
					"Skipping " .. node:name() .. ": name not recognized. "
					.. "Node name must contain a body part keyword (belt/torso/waist/core/"
					.. "head/foot/toe/hand/arm/shoulder/leg/thigh/hip/knee/cover/cape) "
					.. "with optional left/right indicators."
				)
			end
		end
	end

	if animation == "all" then
		-- Create each animation type as a separate named animation
		for name, func in pairs(animationMapping) do
			createAnimation(animatableNodes, context, name, func)
		end
	else
		local animFunc = animationMapping[animation]
		if animFunc == nil then
			error("No animation callback registered for: " .. animation)
		end
		createAnimation(animatableNodes, context, animation, animFunc)
	end

	g_scenegraph.updateTransforms()
end
