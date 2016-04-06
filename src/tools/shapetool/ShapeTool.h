#pragma once

#include "ui/UIApp.h"

class ShapeTool: public ui::UIApp {
protected:
public:
	ShapeTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	~ShapeTool();

	core::AppState onRunning() override;
};
