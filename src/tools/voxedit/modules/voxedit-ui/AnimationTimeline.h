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
	void update(const char *sequencerTitle, ImGuiID dockId);
};

} // namespace voxedit
