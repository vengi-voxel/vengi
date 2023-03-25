/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

class AnimationTimeline;

/**
 * @brief The animation panel will open all available animations for a model and allows you to switch
 * the animation.
 */
class AnimationPanel {
private:
	core::String _newAnimation;
public:

	void update(const char *title, command::CommandExecutionListener &listener, AnimationTimeline *animationTimeline);
};

} // namespace voxedit
