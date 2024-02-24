/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "command/CommandHandler.h"

namespace voxedit {

class MementoPanel : public ui::Panel {
public:
	PANEL_CLASS(MementoPanel)
	void update(const char *title, command::CommandExecutionListener &listener);
};

} // namespace voxedit
