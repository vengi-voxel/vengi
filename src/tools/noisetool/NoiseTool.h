/**
 * @file
 */

#pragma once

#include "ui/UIApp.h"

class NoiseToolWindow;

// TODO: render a 2d graph from -1 to 1
// TODO: implement scale module
// TODO: implement combinations

/**
 * @brief This tool provides a UI to create noise images on-the-fly.
 */
class NoiseTool: public ui::UIApp {
private:
	NoiseToolWindow* _window = nullptr;
public:
	NoiseTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	core::AppState onInit() override;
	core::AppState onRunning() override;
};
