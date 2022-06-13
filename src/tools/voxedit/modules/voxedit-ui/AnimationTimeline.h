/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "imgui.h"

namespace voxedit {

class AnimationTimeline {
private:
	int _lastActiveNode = -1;
public:
	void update(const char *sequencerTitle, ImGuiID dockIdMainDown);
};

} // namespace voxedit
