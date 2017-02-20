/**
 * @file
 */

#pragma once

#include "ui/UIApp.h"
#include "NoiseData.h"
#include <unordered_map>

class NoiseToolWindow;

// TODO: implement combinations

/**
 * @brief This tool provides a UI to create noise images on-the-fly.
 */
class NoiseTool: public ui::UIApp {
private:
	std::unordered_map<uint32_t, NoiseData> _noiseData;

	NoiseToolWindow* _window = nullptr;
public:
	NoiseTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	void add(uint32_t dataId, const NoiseData& data);
	void remove(uint32_t dataId);

	core::AppState onInit() override;
	core::AppState onRunning() override;
};
