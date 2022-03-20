/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"

namespace voxedit {

class PositionsPanel {
private:
	void modelView(command::CommandExecutionListener &listener);
	void sceneView(command::CommandExecutionListener &listener, int frame);
public:
	void update(const char *title, command::CommandExecutionListener &listener, int frame);
};

}
