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
		{ name = 'footAngleFactor', desc = 'How much the foot/leg should be rotated', type = 'float', default = 0.5, min = 0.0, max = 3.0},
		{ name = 'tposeInput', desc = 'Input model has arms in T-pose (horizontal). Arms are rotated to neutral (hanging) before animating.', type = 'bool', default = 'false' }
	}
end

function description()
	return "Animate a given scene if the nodes are named correctly. "
		.. "Node names must contain a body part keyword (belt/torso/waist/core/head/foot/toe/"
		.. "hand/arm/shoulder/leg/thigh/hip/knee/cover/cape) with optional left/right indicators. "
		.. "Supports upper/lower leg segments (e.g. K_Leg_Left_u, K_Leg_Left_l). "
		.. "Supports walk, run, idle, wave, jump and t-pose. "
		.. "Set tposeInput=true if the model's arms are extended horizontally."
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
-- Base transform helpers.
--
-- Every animation composes DELTAS on top of the rest-pose transform so that
-- the parent-child local translations and orientations are preserved.
--
-- For T-pose input models (tposeInput=true), arm/shoulder/hand parts get
-- an additional Z rotation that brings the arms from horizontal to hanging
-- at the sides before any animation delta is applied.
---------------------------------------------------------------------------

-- Capture each node's frame-0 local translation and orientation (rest pose).
-- Must be called BEFORE clearing keyframes.
local function captureBaseTransforms(nodes)
	local base = {}
	for _, node in ipairs(nodes) do
		if node:hasKeyFrameForFrame(0) then
			local kf = node:keyFrameForFrame(0)
			local lt = kf:localTranslation()
			local lo = kf:localOrientation()
			base[node:id()] = {
				translation = { x = lt.x, y = lt.y, z = lt.z },
				orientation = lo,
			}
		end
	end
	return base
end

-- Return true if the part is an arm / shoulder / hand
local function isArmPart(part)
	return part == "left_shoulder" or part == "right_shoulder"
		or part == "left_arm" or part == "right_arm"
		or part == "left_hand" or part == "right_hand"
end

local function isLeftArm(part)
	return part == "left_shoulder" or part == "left_arm" or part == "left_hand"
end

-- Compute the "neutral" orientation for a node.
-- For non-arm parts this is just the rest-pose orientation.
-- For arm parts of a T-pose model it additionally includes a Z rotation
-- that brings the arm from horizontal to hanging at the character's sides.
local function getNeutralQ(part, context, nodeId)
	local base = context.baseTransforms[nodeId]
	local baseQ = (base and base.orientation) or g_quat.new()

	if context.tposeInput and isArmPart(part) then
		if isLeftArm(part) then
			-- left arm extends +X, rotate -90° Z to hang down (-Y)
			return g_quat.rotateZ(-math.pi / 2.0) * baseQ
		else
			-- right arm extends -X, rotate +90° Z to hang down (-Y)
			return g_quat.rotateZ(math.pi / 2.0) * baseQ
		end
	end

	return baseQ
end

-- Compose a delta rotation on top of the keyframe's current orientation.
-- The kf was already initialized with the neutral orientation, so:
--   final = deltaQ * neutralQ
local function composeOrientation(kf, deltaQ)
	kf:setLocalOrientation(deltaQ * kf:localOrientation())
end

-- Add a translation offset to the keyframe's current translation.
-- The kf was already initialized with the base translation, so:
--   final = base + delta
local function addTranslation(kf, dx, dy, dz)
	local t = kf:localTranslation()
	kf:setLocalTranslation(t.x + dx, t.y + dy, t.z + dz)
end

---------------------------------------------------------------------------
-- Walk animation
--
-- Natural walk cycle: opposite arm/leg pairs swing together.
-- Right foot forward = left arm forward and vice-versa.
-- Full cycle over 2*pi so frame 0 == frame N for seamless looping.
--
-- Animation function signature: (node, kf, part, keyframe, context)
--   node     - the scene graph node
--   kf       - keyframe (already initialized with base transform)
--   part     - body part key from identifyBodyPart()
--   keyframe - keyframe index (0 .. maxKeyFrames-1)
--   context  - shared context table
---------------------------------------------------------------------------
local function walkAnimation(node, kf, part, keyframe, context)
	local phase = (keyframe / context.maxKeyFrames) * math.pi * 2.0
	local sinPhase = math.sin(phase)

	local footSwing = context.footAngleFactor * sinPhase
	local armSwing  = context.handAngleFactor * sinPhase

	if part == "torso" or part == "cover" then
		-- subtle sway and vertical bob
		local sway = 0.04 * sinPhase
		local bob  = 0.015 * math.abs(math.sin(phase * 2.0))
		composeOrientation(kf, g_quat.rotateY(sway))
		addTranslation(kf, 0, bob, 0)

	elseif part == "head" then
		-- very subtle head bob, counter the torso sway slightly
		local headBob = 0.02 * math.sin(phase * 2.0)
		local headSway = -0.02 * sinPhase
		composeOrientation(kf, g_quat.rotateXY(headBob, headSway))

	elseif part == "right_upper_leg" then
		composeOrientation(kf, g_quat.rotateX(footSwing))

	elseif part == "left_upper_leg" then
		composeOrientation(kf, g_quat.rotateX(-footSwing))

	elseif part == "right_lower_leg" then
		composeOrientation(kf, g_quat.rotateX(footSwing * 0.6))

	elseif part == "left_lower_leg" then
		composeOrientation(kf, g_quat.rotateX(-footSwing * 0.6))

	elseif part == "right_knee" then
		composeOrientation(kf, g_quat.rotateX(footSwing * 0.3))

	elseif part == "left_knee" then
		composeOrientation(kf, g_quat.rotateX(-footSwing * 0.3))

	elseif part == "right_foot" then
		composeOrientation(kf, g_quat.rotateX(footSwing * 0.4))

	elseif part == "left_foot" then
		composeOrientation(kf, g_quat.rotateX(-footSwing * 0.4))

	elseif part == "right_toe" then
		composeOrientation(kf, g_quat.rotateX(footSwing * 0.2))

	elseif part == "left_toe" then
		composeOrientation(kf, g_quat.rotateX(-footSwing * 0.2))

	elseif part == "right_shoulder" or part == "right_arm" or part == "right_hand" then
		-- right arm swings opposite to right leg
		composeOrientation(kf, g_quat.rotateX(-armSwing))

	elseif part == "left_shoulder" or part == "left_arm" or part == "left_hand" then
		composeOrientation(kf, g_quat.rotateX(armSwing))
	end
end

---------------------------------------------------------------------------
-- Run animation
--
-- Similar to walk but larger amplitudes, more forward lean, more bounce.
---------------------------------------------------------------------------
local function runAnimation(node, kf, part, keyframe, context)
	local phase = (keyframe / context.maxKeyFrames) * math.pi * 2.0
	local sinPhase = math.sin(phase)

	local footSwing = context.footAngleFactor * 1.8 * sinPhase
	local armSwing  = context.handAngleFactor * 1.6 * sinPhase

	if part == "torso" or part == "cover" then
		-- forward lean + bounce
		local lean = 0.15
		local sway = 0.06 * sinPhase
		local bounce = 0.04 * math.abs(math.sin(phase * 2.0))
		composeOrientation(kf, g_quat.rotateXY(lean, sway))
		addTranslation(kf, 0, bounce, 0)

	elseif part == "head" then
		-- counter the forward lean, slight bob
		local counterLean = -0.1
		local headBob = 0.03 * math.sin(phase * 2.0)
		composeOrientation(kf, g_quat.rotateX(counterLean + headBob))

	elseif part == "right_upper_leg" then
		local bendBack = math.max(0, -sinPhase) * 0.3
		composeOrientation(kf, g_quat.rotateX(footSwing + bendBack))

	elseif part == "left_upper_leg" then
		local bendBack = math.max(0, sinPhase) * 0.3
		composeOrientation(kf, g_quat.rotateX(-footSwing + bendBack))

	elseif part == "right_lower_leg" then
		local bendBack = math.max(0, -sinPhase) * 0.2
		composeOrientation(kf, g_quat.rotateX(footSwing * 0.6 + bendBack))

	elseif part == "left_lower_leg" then
		local bendBack = math.max(0, sinPhase) * 0.2
		composeOrientation(kf, g_quat.rotateX(-footSwing * 0.6 + bendBack))

	elseif part == "right_knee" then
		composeOrientation(kf, g_quat.rotateX(footSwing * 0.3))

	elseif part == "left_knee" then
		composeOrientation(kf, g_quat.rotateX(-footSwing * 0.3))

	elseif part == "right_foot" then
		local bendBack = math.max(0, -sinPhase) * 0.15
		composeOrientation(kf, g_quat.rotateX(footSwing * 0.4 + bendBack))

	elseif part == "left_foot" then
		local bendBack = math.max(0, sinPhase) * 0.15
		composeOrientation(kf, g_quat.rotateX(-footSwing * 0.4 + bendBack))

	elseif part == "right_toe" then
		composeOrientation(kf, g_quat.rotateX(footSwing * 0.2))

	elseif part == "left_toe" then
		composeOrientation(kf, g_quat.rotateX(-footSwing * 0.2))

	elseif part == "right_shoulder" or part == "right_arm" or part == "right_hand" then
		-- arms pump with some elbow bend via Z
		local elbowBend = 0.2 + 0.15 * math.max(0, sinPhase)
		composeOrientation(kf, g_quat.rotateX(-armSwing) * g_quat.rotateZ(-elbowBend))

	elseif part == "left_shoulder" or part == "left_arm" or part == "left_hand" then
		local elbowBend = 0.2 + 0.15 * math.max(0, -sinPhase)
		composeOrientation(kf, g_quat.rotateX(armSwing) * g_quat.rotateZ(elbowBend))
	end
end

---------------------------------------------------------------------------
-- Idle / breathing animation
--
-- Subtle breathing motion: torso rises/falls, arms sway gently.
---------------------------------------------------------------------------
local function idleAnimation(node, kf, part, keyframe, context)
	local phase = (keyframe / context.maxKeyFrames) * math.pi * 2.0
	local sinPhase = math.sin(phase)

	if part == "torso" or part == "cover" then
		-- breathing: slight rise and fall
		local breathe = 0.012 * sinPhase
		addTranslation(kf, 0, breathe, 0)

	elseif part == "head" then
		-- very subtle look around
		local lookY = 0.03 * math.sin(phase * 0.5)
		composeOrientation(kf, g_quat.rotateY(lookY))

	elseif part == "right_shoulder" or part == "right_arm" or part == "right_hand" then
		-- arms hang naturally, slight sway
		local sway = 0.02 * sinPhase
		composeOrientation(kf, g_quat.rotateZ(-sway))

	elseif part == "left_shoulder" or part == "left_arm" or part == "left_hand" then
		local sway = 0.02 * sinPhase
		composeOrientation(kf, g_quat.rotateZ(sway))
	end
	-- legs and other parts: keep the initialized base values (no change)
end

---------------------------------------------------------------------------
-- Wave animation
--
-- Right hand waves while the rest of the body stays mostly still.
---------------------------------------------------------------------------
local function waveAnimation(node, kf, part, keyframe, context)
	local phase = (keyframe / context.maxKeyFrames) * math.pi * 2.0
	local sinPhase = math.sin(phase)

	if part == "right_shoulder" or part == "right_arm" then
		-- raise right arm forward and up, then wave side to side
		local raise = 2.0  -- arm forward and above horizontal (~115 degrees)
		local wave = 0.4 * sinPhase
		composeOrientation(kf, g_quat.rotateX(raise) * g_quat.rotateZ(wave))

	elseif part == "right_hand" then
		-- wrist wave back and forth
		local wristWave = 0.5 * math.sin(phase * 2.0)
		composeOrientation(kf, g_quat.rotateZ(wristWave))

	elseif part == "torso" or part == "cover" then
		-- slight lean to compensate for raised arm
		local lean = 0.04 * sinPhase
		composeOrientation(kf, g_quat.rotateZ(lean))

	elseif part == "head" then
		-- look slightly toward the waving hand direction
		local look = 0.1 * sinPhase
		composeOrientation(kf, g_quat.rotateY(look))
	end
	-- everything else: keep the initialized base values
end

---------------------------------------------------------------------------
-- Jump animation
--
-- Anticipation -> launch -> air -> landing sequence.
-- Uses normalized time (0-1) to drive different phases.
---------------------------------------------------------------------------

-- smooth step helper (module-level to avoid per-call closure)
local function smoothstep(edge0, edge1, x)
	local v = math.max(0, math.min(1, (x - edge0) / (edge1 - edge0)))
	return v * v * (3 - 2 * v)
end

local function jumpAnimation(node, kf, part, keyframe, context)
	-- normalized time [0, 1)
	local t = keyframe / context.maxKeyFrames

	-- Phase breakdown:
	-- 0.0 - 0.2: anticipation (crouch down)
	-- 0.2 - 0.4: launch (extend upward)
	-- 0.4 - 0.7: airborne (arms/legs spread)
	-- 0.7 - 1.0: landing (absorb impact)

	if part == "torso" or part == "cover" then
		local crouch
		local leanFwd
		if t < 0.2 then
			crouch = -smoothstep(0, 0.2, t) * 0.5
			leanFwd = smoothstep(0, 0.2, t) * 0.2
		elseif t < 0.4 then
			crouch = -0.5 + smoothstep(0.2, 0.4, t) * 1.5
			leanFwd = 0.2 - smoothstep(0.2, 0.4, t) * 0.2
		elseif t < 0.7 then
			crouch = 1.0 - smoothstep(0.4, 0.7, t) * 0.3
			leanFwd = 0
		else
			crouch = 0.7 - smoothstep(0.7, 1.0, t) * 1.0
			leanFwd = smoothstep(0.7, 0.9, t) * 0.15 - smoothstep(0.9, 1.0, t) * 0.15
		end
		addTranslation(kf, 0, crouch, 0)
		composeOrientation(kf, g_quat.rotateX(leanFwd))

	elseif part == "head" then
		local counterLean = 0
		if t < 0.2 then
			counterLean = -smoothstep(0, 0.2, t) * 0.15
		elseif t < 0.4 then
			counterLean = -0.15 + smoothstep(0.2, 0.4, t) * 0.15
		end
		composeOrientation(kf, g_quat.rotateX(counterLean))

	elseif part == "right_upper_leg" or part == "right_foot" then
		local bend
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
		composeOrientation(kf, g_quat.rotateX(bend))

	elseif part == "left_upper_leg" or part == "left_foot" then
		local bend
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
		composeOrientation(kf, g_quat.rotateX(bend))

	elseif part == "right_lower_leg" or part == "right_knee" or part == "right_toe" then
		local bend
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
		composeOrientation(kf, g_quat.rotateX(bend))

	elseif part == "left_lower_leg" or part == "left_knee" or part == "left_toe" then
		local bend
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
		composeOrientation(kf, g_quat.rotateX(bend))

	elseif part == "right_shoulder" or part == "right_arm" or part == "right_hand" then
		local raise
		if t < 0.2 then
			raise = smoothstep(0, 0.2, t) * 0.3
		elseif t < 0.4 then
			raise = 0.3 - smoothstep(0.2, 0.4, t) * 1.2
		elseif t < 0.7 then
			raise = -0.9
		else
			raise = -0.9 + smoothstep(0.7, 1.0, t) * 0.9
		end
		composeOrientation(kf, g_quat.rotateX(raise))

	elseif part == "left_shoulder" or part == "left_arm" or part == "left_hand" then
		local raise
		if t < 0.2 then
			raise = smoothstep(0, 0.2, t) * 0.3
		elseif t < 0.4 then
			raise = 0.3 - smoothstep(0.2, 0.4, t) * 1.2
		elseif t < 0.7 then
			raise = -0.9
		else
			raise = -0.9 + smoothstep(0.7, 1.0, t) * 0.9
		end
		composeOrientation(kf, g_quat.rotateX(raise))
	end
end

---------------------------------------------------------------------------
-- T-Pose
--
-- Creates a standard T-pose animation. Only needs a single keyframe.
-- Arms are forced horizontal; everything else keeps the rest pose.
-- This animation sets ABSOLUTE orientations (not deltas) for arms.
---------------------------------------------------------------------------
local function tposeAnimation(node, kf, part, keyframe, context)
	-- T-pose only needs frame 0
	if keyframe > 0 then
		return
	end

	if part == "right_shoulder" or part == "right_arm" then
		-- arm straight out to the right: rotate -90 degrees around Z
		kf:setLocalOrientation(g_quat.rotateZ(-math.pi / 2.0))
	elseif part == "left_shoulder" or part == "left_arm" then
		-- arm straight out to the left: rotate +90 degrees around Z
		kf:setLocalOrientation(g_quat.rotateZ(math.pi / 2.0))
	end
	-- Everything else: keep the base orientation and translation (already initialized)
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
	end

	-- Capture base transforms from current frame 0 (rest pose)
	-- BEFORE clearing keyframes so we preserve the original values.
	context.baseTransforms = captureBaseTransforms(nodes)

	if context.createAnim then
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

	-- For the "tpose" output animation we do NOT apply the tpose-to-neutral
	-- adjustment; the tpose function sets absolute arm orientations instead.
	local applyNeutral = (animName ~= "tpose")

	for _, node in ipairs(nodes) do
		local part = identifyBodyPart(node:name())
		if part ~= nil then
			-- Compute the neutral orientation used to initialize each keyframe
			local base = context.baseTransforms[node:id()]
			local neutralQ
			if applyNeutral then
				neutralQ = getNeutralQ(part, context, node:id())
			else
				neutralQ = (base and base.orientation) or g_quat.new()
			end

			for keyframe = 0, maxKF - 1 do
				local frame = keyframe * context.frameDuration
				local kf = addOrGetKeyFrame(node, frame)

				-- Initialize keyframe with base translation and neutral orientation
				if base then
					kf:setLocalTranslation(
						base.translation.x, base.translation.y, base.translation.z)
				end
				kf:setLocalOrientation(neutralQ)

				-- Apply animation-specific deltas
				animFunc(node, kf, part, keyframe, context)
			end
		end
	end
end

function main(_, _, _, animation, createAnim, maxKeyFrames, frameDuration, handAngleFactor, footAngleFactor, tposeInput)
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
		tposeInput = tposeInput,
		baseTransforms = {},
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
