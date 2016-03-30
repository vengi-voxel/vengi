#pragma once

#include "core/App.h"

class ShaderTool: public core::App {
protected:
public:
	ShaderTool(io::FilesystemPtr filesystem, core::EventBusPtr eventBus);
	~ShaderTool();

	core::AppState onRunning() override;
};
