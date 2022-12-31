/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "imgui.h"

namespace voxedit {

class AnimationTimeline {
private:
	bool _play = false;
public:
	bool update(const char *sequencerTitle);
};

} // namespace voxedit
