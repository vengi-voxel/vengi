/**
 * @file
 */

#pragma once

#include "ui/UIApp.h"

class NoiseTool: public ui::UIApp {
public:
	NoiseTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus);

	core::AppState onInit() override;
};
