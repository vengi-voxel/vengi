--
-- Shared module for identifying body parts from node names.
-- Used by animate.lua, setpivot.lua, and other character scripts.
--
-- Body part keywords: belt, torso, waist, body, chest, spine, core, head,
--   foot, toe, hand, arm, shoulder, leg, thigh, hip, knee, cover, cape, cloak.
-- Side indicators: left/right or _l/_r prefix/suffix.
-- Segment indicators: _u (upper) / _l (lower) for legs (when side is already
--   determined by left/right keyword).
--
-- e.g. belt, arm_left, K_Arm_Left, right_leg, K_Leg_Right_u, shoulder_r
--

local module = {}

-- Determine the side (left/right) from the lowered name
function module.detectSide(n)
	if n:find("left") then return "left" end
	if n:find("right") then return "right" end
	-- Fallback: short-form indicators (only if left/right not found)
	if n:match("_l$") or n:match("^l_") then return "left" end
	if n:match("_r$") or n:match("^r_") then return "right" end
	return nil
end

-- Map a node name to a body-part key (or nil if unrecognized)
--
-- Returns one of:
--   "torso", "cover", "head",
--   "left_shoulder", "right_shoulder",
--   "left_hand", "right_hand",
--   "left_arm", "right_arm",
--   "left_toe", "right_toe",
--   "left_foot", "right_foot",
--   "left_knee", "right_knee",
--   "left_upper_leg", "right_upper_leg",
--   "left_lower_leg", "right_lower_leg",
--   or nil
function module.identify(name)
	local n = string.lower(name)
	local side = module.detectSide(n)

	-- Torso family (no side needed)
	if n:find("waist") or n:find("belt") or n:find("torso") or n:find("body")
		or n:find("chest") or n:find("spine") or n:find("core") then
		return "torso"
	end

	-- Cover / armor / cape accessory (follows torso)
	if n:find("cover") or n:find("cape") or n:find("cloak") or n:find("armor") then
		return "cover"
	end

	-- Head (no side needed)
	if n:find("head") then return "head" end

	-- Shoulder (check before arm to avoid substring issues)
	if n:find("shoulder") then
		if side == "left" then return "left_shoulder" end
		return "right_shoulder"
	end

	-- Hand (check before arm)
	if n:find("hand") then
		if side == "left" then return "left_hand" end
		return "right_hand"
	end

	-- Arm
	if n:find("arm") then
		if side == "left" then return "left_arm" end
		return "right_arm"
	end

	-- Toe (check before foot to keep them distinct)
	if n:find("toe") then
		if side == "left" then return "left_toe" end
		return "right_toe"
	end

	-- Foot
	if n:find("foot") then
		if side == "left" then return "left_foot" end
		return "right_foot"
	end

	-- Knee (check before generic leg)
	if n:find("knee") then
		if side == "left" then return "left_knee" end
		return "right_knee"
	end

	-- Leg, thigh, hip, shin
	if n:find("leg") or n:find("thigh") or n:find("hip") then
		-- Distinguish upper/lower when side is already set by left/right keyword
		if n:find("left") or n:find("right") then
			local isLower = n:match("_l$") or n:find("lower") or n:find("shin")
			if isLower then
				return side == "left" and "left_lower_leg" or "right_lower_leg"
			end
		end
		-- Default: upper leg (or unspecified leg without segment suffix)
		if side == "left" then return "left_upper_leg" end
		return "right_upper_leg"
	end

	return nil
end

-- The message shown when a node name is not recognized
module.skipMessage = "Node name must contain a body part keyword "
	.. "(belt/torso/waist/core/head/foot/toe/hand/arm/shoulder/leg/thigh/hip/knee/cover/cape) "
	.. "with optional left/right indicators."

return module
