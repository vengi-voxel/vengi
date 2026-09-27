/**
 * @file
 */

#include "SceneGraphBodyPart.h"
#include "core/StringUtil.h"

namespace scenegraph {

static bool containsFold(const core::String &n, const char *needle) {
	return n.find(needle) != core::String::npos;
}

core::String detectBodyPartSide(const core::String &name) {
	const core::String n = name.toLower();
	if (containsFold(n, "left")) {
		return "left";
	}
	if (containsFold(n, "right")) {
		return "right";
	}
	if (core::string::endsWith(n, core::String("_l")) || core::string::startsWith(n, "l_")) {
		return "left";
	}
	if (core::string::endsWith(n, core::String("_r")) || core::string::startsWith(n, "r_")) {
		return "right";
	}
	return core::String();
}

core::String identifyBodyPart(const core::String &name) {
	const core::String n = name.toLower();
	const core::String side = detectBodyPartSide(name);

	if (containsFold(n, "waist") || containsFold(n, "belt") || containsFold(n, "torso") || containsFold(n, "body") ||
		containsFold(n, "chest") || containsFold(n, "spine") || containsFold(n, "core")) {
		return "torso";
	}

	if (containsFold(n, "cover") || containsFold(n, "cape") || containsFold(n, "cloak") || containsFold(n, "armor")) {
		return "cover";
	}

	if (containsFold(n, "head")) {
		return "head";
	}

	if (containsFold(n, "shoulder")) {
		return side == "left" ? "left_shoulder" : "right_shoulder";
	}

	if (containsFold(n, "hand")) {
		return side == "left" ? "left_hand" : "right_hand";
	}

	if (containsFold(n, "arm")) {
		return side == "left" ? "left_arm" : "right_arm";
	}

	if (containsFold(n, "toe")) {
		return side == "left" ? "left_toe" : "right_toe";
	}

	if (containsFold(n, "foot")) {
		return side == "left" ? "left_foot" : "right_foot";
	}

	if (containsFold(n, "knee")) {
		return side == "left" ? "left_knee" : "right_knee";
	}

	if (containsFold(n, "leg") || containsFold(n, "thigh") || containsFold(n, "hip")) {
		if (containsFold(n, "left") || containsFold(n, "right")) {
			const bool isLower =
				core::string::endsWith(n, core::String("_l")) || containsFold(n, "lower") || containsFold(n, "shin");
			if (isLower) {
				return side == "left" ? "left_lower_leg" : "right_lower_leg";
			}
		}
		return side == "left" ? "left_upper_leg" : "right_upper_leg";
	}

	return core::String();
}

const char *identifyBodyPartSkipMessage() {
	return "Node name must contain a body part keyword "
		   "(belt/torso/waist/core/head/foot/toe/hand/arm/shoulder/leg/thigh/hip/knee/cover/cape) "
		   "with optional left/right indicators.";
}

} // namespace scenegraph
