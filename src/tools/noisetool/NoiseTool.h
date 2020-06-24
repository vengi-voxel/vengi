/**
 * @file
 */

#pragma once

#include "ui/imgui/IMGUIApp.h"
#include "NoiseData.h"
#include "noise/Noise.h"
#include "core/Common.h"
#include "core/collection/ConcurrentQueue.h"
#include "core/collection/StringMap.h"

/**
 * @brief This tool provides a UI to create noise images on-the-fly.
 *
 * @ingroup Tools
 */
class NoiseTool: public ui::imgui::IMGUIApp {
private:
	using Super = ui::imgui::IMGUIApp;
	static constexpr int BPP = 4;
	static_assert(BPP == sizeof(uint32_t), "This code heavily relies on RGBA being 32bit");
	noise::Noise _noise;

	struct QueueData {
		NoiseData data;
		uint8_t *noiseBuffer;
		uint8_t *graphBuffer;

		inline bool operator<(const QueueData& rhs) const {
			return noiseBuffer < rhs.noiseBuffer;
		}
	};
	core::ConcurrentQueue<QueueData> _queue;
	NoiseData _currentData;
	core::StringMap<NoiseData> _noiseDataMap;

	int _noiseWidth = 768;
	int _noiseHeight = 1024;
	const int _graphHeight = 65;
	uint8_t *_graphBufferBackground = nullptr;

	/**
	 * @return the noise in the range [-1.0 - 1.0]
	 */
	float getNoise(int x, int y, const NoiseData& _data);
	int index(int x, int y) const;
	void updateForNoiseType(NoiseType type);
	void generateAll();

public:
	NoiseTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	void generateImage(NoiseType type);

	void onRenderUI() override;

	core::AppState onInit() override;
	core::AppState onRunning() override;
	core::AppState onCleanup() override;
};

inline int NoiseTool::index(int x, int y) const {
	core_assert_msg(x >= 0, "x is smaller than 0: %i", x);
	core_assert_msg(x < _noiseWidth * BPP, "x is out of bounds: %i", x);
	core_assert_msg(y >= 0, "y is smaller than 0: %i", y);
	return x * BPP + y * _noiseWidth * BPP;
}
