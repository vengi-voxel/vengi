/**
 * @file
 */

#pragma once

#include "ui/UIApp.h"
#include "NoiseData.h"
#include <map>

class NoiseItemSource;

typedef std::map<uint32_t, NoiseData> NoiseDataMap;

// TODO: implement combinations

/**
 * @brief This tool provides a UI to create noise images on-the-fly.
 */
class NoiseTool: public ui::UIApp {
private:
	NoiseDataMap _noiseData;
	NoiseItemSource* _noiseItemSource = nullptr;
public:
	NoiseTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	void add(uint32_t dataId, const NoiseData& data);
	void remove(uint32_t dataId);

	NoiseItemSource* noiseItemSource();

	const NoiseDataMap& noiseData() const;

	core::AppState onInit() override;
	core::AppState onRunning() override;
};

inline const NoiseDataMap& NoiseTool::noiseData() const {
	return _noiseData;
}

inline NoiseItemSource* NoiseTool::noiseItemSource() {
	return _noiseItemSource;
}
