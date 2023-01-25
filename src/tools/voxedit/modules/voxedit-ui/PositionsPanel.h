/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"

namespace voxedit {

class PositionsPanel {
private:
	bool _lastChanged = false;
	void modelView(command::CommandExecutionListener &listener);
	void sceneView(command::CommandExecutionListener &listener);
public:
	void update(const char *title, bool sceneMode, command::CommandExecutionListener &listener);
};

}
