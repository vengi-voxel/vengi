/**
 * @file
 */

#pragma once

#include "ui/turbobadger/UIApp.h"
#include "NoiseData.h"
#include <map>

class NoiseItemSource;
class NoiseToolWindow;

typedef std::map<uint32_t, NoiseData> NoiseDataMap;

// TODO: implement combinations

/**
 * @brief This tool provides a UI to create noise images on-the-fly.
 *
 * @ingroup Tools
 */
class NoiseTool: public ui::turbobadger::UIApp {
private:
	using Super = ui::turbobadger::UIApp;
	NoiseDataMap _noiseData;
	NoiseToolWindow* _window = nullptr;
	NoiseItemSource* _noiseItemSource = nullptr;
public:
	NoiseTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	void add(uint32_t dataId, const NoiseData& data);
	void remove(uint32_t dataId);

	NoiseItemSource* noiseItemSource();

	const NoiseDataMap& noiseData() const;

	app::AppState onInit() override;
	void onRenderUI() override;
};

inline const NoiseDataMap& NoiseTool::noiseData() const {
	return _noiseData;
}

inline NoiseItemSource* NoiseTool::noiseItemSource() {
	return _noiseItemSource;
}
