/**
 * @file
 */

#pragma once

#include "ui/UIApp.h"

/**
 * @brief This tool provides a UI to create noise images on-the-fly.
 */
class NoiseTool: public ui::UIApp {
public:
	NoiseTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onInit() override;
};
