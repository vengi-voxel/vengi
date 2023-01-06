/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"

namespace voxedit {

/**
 * @brief The animation panel will open all available animations for a model and allows you to switch
 * the animation.
 */
class AnimationPanel {
public:
	void update(const char *title, command::CommandExecutionListener &listener);
};

} // namespace voxedit
