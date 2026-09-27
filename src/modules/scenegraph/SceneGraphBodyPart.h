/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace scenegraph {

/**
 * @brief Map a node name to an animate.lua body-part key.
 *
 * Keywords: belt, torso, waist, body, chest, spine, core, head, foot, toe,
 * hand, arm, shoulder, leg, thigh, hip, knee, cover, cape, cloak, armor.
 * Side: left/right or @c _l / @c _r prefix/suffix. Legs may use @c _u / @c _l
 * (or lower/shin) when a left/right word is already present.
 *
 * @return One of @c torso, @c cover, @c head, @c left_shoulder, @c right_shoulder,
 * @c left_hand, @c right_hand, @c left_arm, @c right_arm, @c left_toe, @c right_toe,
 * @c left_foot, @c right_foot, @c left_knee, @c right_knee, @c left_upper_leg,
 * @c right_upper_leg, @c left_lower_leg, @c right_lower_leg - or empty if unrecognized.
 */
core::String identifyBodyPart(const core::String &name);

/**
 * @brief @c "left", @c "right", or empty if the name has no side marker.
 */
core::String detectBodyPartSide(const core::String &name);

const char *identifyBodyPartSkipMessage();

} // namespace scenegraph
