/**
 * @file
 */

#include "NoiseTool.h"
#include "core/App.h"
#include "dearimgui/imgui.h"
#include "image/Image.h"
#include "core/io/Filesystem.h"
#include "core/metric/Metric.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"
#include "core/Color.h"
#include "core/concurrent/ThreadPool.h"
#include "math/Rect.h"
#include "noise/PoissonDiskDistribution.h"
#include "noise/Simplex.h"
#include "video/Texture.h"
#include "video/TextureConfig.h"
#include "video/Types.h"

#define IMAGE_PREFIX "2d"
#define GRAPH_PREFIX "graph"

NoiseTool::NoiseTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "noisetool");
}

core::AppState NoiseTool::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (!_noise.init()) {
		return core::AppState::InitFailure;
	}

	return state;
}

void NoiseTool::onRenderUI() {
	ImGui::Begin("NoiseTool", nullptr, ImGuiWindowFlags_NoDecoration);

	if (ImGui::Begin("Settings")) {
		ImGui::InputFloat("separation", &_currentData.separation);
		ImGui::InputFloat("seed", &_currentData.separation);
		ImGui::InputInt("seed", &_currentData.seed );
		ImGui::InputFloat("offset", &_currentData.offset);
		ImGui::InputFloat("lacunarity", &_currentData.lacunarity);
		ImGui::InputInt("octaves", &_currentData.octaves);
		ImGui::InputFloat("gain", &_currentData.gain);
		ImGui::InputFloat("frequency", &_currentData.frequency);
		ImGui::InputFloat("ridgedoffset", &_currentData.ridgedOffset);
		ImGui::Checkbox("enabledistance", &_currentData.enableDistance);
	}
	ImGui::End();

	ImGui::Separator();
	if (ImGui::Button("Quit")) {
		requestQuit();
	}
	ImGui::End();
}

core::AppState NoiseTool::onRunning() {
	core::AppState state = Super::onRunning();
	QueueData qd;
	if (!_queue.pop(qd)) {
		return state;
	}
	NoiseData& data = qd.data;
	const core::String& key = core::string::format("-%f-%i-%f-%i-%f-%i-%f-%f-%f",
			data.ridgedOffset, (int)data.noiseType, data.separation, (int)data.enableDistance,
			data.offset, data.octaves, data.lacunarity, data.gain, data.frequency);

	video::TextureConfig cfg;
	cfg.format(video::TextureFormat::RGBA);

	data.noise = video::createTexture(cfg);
	data.graph = video::createTexture(cfg);
	data.noise->upload(_noiseWidth, _noiseHeight, qd.noiseBuffer);
	data.graph->upload(_noiseWidth, _graphHeight, qd.graphBuffer);

	_noiseDataMap.put(key, data);

	Log::info("Generating noise for %s took %" SDL_PRIu64 "ms", getNoiseTypeName(data.noiseType), data.endmillis - data.millis);
	delete [] qd.noiseBuffer;
	delete [] qd.graphBuffer;
	return state;
}

core::AppState NoiseTool::onCleanup() {
	delete[] _graphBufferBackground;
	_graphBufferBackground = nullptr;
	_noise.shutdown();
	return Super::onCleanup();
}

void NoiseTool::generateAll() {
	for (int i = 0; i < (int)NoiseType::Max; ++i) {
		NoiseType type = (NoiseType)i;
		generateImage(type);
	}
}

void NoiseTool::generateImage(NoiseType type) {
	core_trace_scoped(GenerateImage);
	Log::info("Generate noise for %s", getNoiseTypeName(type));
	_currentData.noiseType = type;

	NoiseData data = _currentData;
	threadPool().enqueue([this, data] () {
		const size_t noiseBufferSize = _noiseWidth * _noiseHeight * BPP;
		const size_t graphBufferSize = _noiseWidth * _graphHeight * BPP;
		QueueData qd;
		qd.data = data;
		qd.data.millis = core::TimeProvider::systemMillis();
		qd.noiseBuffer = new uint8_t[noiseBufferSize];
		qd.graphBuffer = new uint8_t[graphBufferSize];

		uint8_t* noiseBuffer = qd.noiseBuffer;
		uint8_t* graphBuffer = qd.graphBuffer;

		core_memset(noiseBuffer, 255, noiseBufferSize);
		core_memcpy(graphBuffer, _graphBufferBackground, graphBufferSize);

		if (qd.data.noiseType == NoiseType::poissonDiskDistribution) {
			const math::Rect<int> area(0, 0, _noiseWidth - 1, _noiseHeight - 1);
			const std::vector<glm::vec2>& distrib = noise::poissonDiskDistribution(qd.data.separation, area);
			for (const glm::vec2& v : distrib) {
				const int x = v.x;
				const int y = v.y;
				uint8_t* buf = &noiseBuffer[index(x, y)];
				*((uint32_t*)buf) = core::Color::getRGBA(core::Color::Black);
			}
		} else {
			const int h = _graphHeight - 1;
			for (int y = 0; y < _noiseHeight; ++y) {
				for (int x = 0; x < _noiseWidth; ++x) {
					const float n = getNoise(x, y, qd.data);
					const float cn = noise::norm(n);
					const uint8_t c = cn * 255;
					uint8_t* buf = &noiseBuffer[index(x, y)];
					core_memset(buf, c, BPP - 1);
					if (y == 0 && x < _noiseWidth) {
						const int gy = h - (cn * h);
						const int idx = index(x, gy);
						uint8_t* gbuf = &graphBuffer[idx];
						*((uint32_t*)gbuf) = core::Color::getRGBA(core::Color::Red);
					}
				}
			}
		}

		qd.data.endmillis = core::TimeProvider::systemMillis();
		this->_queue.push(qd);
	});
}

float NoiseTool::getNoise(int x, int y, const NoiseData& data) {
	const NoiseType noiseType = data.noiseType;
	const glm::vec2 position(data.offset + x * data.frequency, data.offset + y * data.frequency);
	switch (noiseType) {
	case NoiseType::doubleNoise: {
		const glm::ivec3 p3(position.x, position.y, 0);
		return _noise.doubleValueNoise(p3, 0);
	}
	case NoiseType::simplexNoise:
		return noise::noise(position);
	case NoiseType::ridgedNoise:
		return noise::ridgedNoise(position);
	case NoiseType::flowNoise:
		return noise::flowNoise(position, data.millis);
	case NoiseType::fbm:
		return noise::fBm(position, data.octaves, data.lacunarity, data.gain);
	case NoiseType::fbmCascade:
		return noise::fBm(noise::fBm(position));
	case NoiseType::fbmAnalyticalDerivatives:
		return noise::fBm(noise::dfBm(position));
	case NoiseType::flowNoiseFbm: {
		const glm::vec3 p3(position, data.millis * 0.1f);
		const float fbm = noise::fBm(p3, data.octaves, data.lacunarity, data.gain);
		return noise::flowNoise(position + fbm, data.millis);
	}
	case NoiseType::ridgedMFTime: {
		const glm::vec3 p3(position, data.millis * 0.1f);
		return noise::ridgedMF(p3, data.ridgedOffset, data.octaves, data.lacunarity, data.gain);
	}
	case NoiseType::ridgedMF:
		return noise::ridgedMF(position, data.ridgedOffset, data.octaves, data.lacunarity, data.gain);
	case NoiseType::ridgedMFCascade: {
		const float n = noise::ridgedMF(position, data.ridgedOffset, data.octaves, data.lacunarity, data.gain);
		return noise::ridgedMF(n, data.ridgedOffset, data.octaves, data.lacunarity, data.gain);
	}
	case NoiseType::iqNoise:
		return noise::iqMatfBm(position, data.octaves, glm::mat2(2.3f, -1.5f, 1.5f, 2.3f), data.gain);
	case NoiseType::analyticalDerivatives: {
		const glm::vec3& n = noise::dnoise(position);
		return (n.y + n.z) * 0.5f;
	}
	case NoiseType::noiseCurlNoise: {
		const glm::vec2& n = noise::curlNoise(position, data.millis);
		return noise::noise(glm::vec2(position.x + n.x, position.y + n.x));
	}
	case NoiseType::voronoi: {
		const glm::dvec3 p3(position.x, position.y, 0.0);
		return _noise.voronoi(p3, data.enableDistance, 1.0, data.seed);
	}
	case NoiseType::worleyNoise:
		return noise::worleyNoise(position);
	case NoiseType::worleyNoiseFbm:
		return noise::worleyfBm(position, data.octaves, data.lacunarity, data.gain);
	case NoiseType::swissTurbulence:
		return _noise.swissTurbulence(position, 0.0f, data.octaves, data.lacunarity, data.gain);
	case NoiseType::jordanTurbulence:
		// float gain0, float gain, float warp0, float warp, float damp0, float damp, float damp_scale;
		return _noise.jordanTurbulence(position, 0.0f, data.octaves, data.lacunarity, data.gain);
	case NoiseType::poissonDiskDistribution:
	case NoiseType::Max:
		break;
	}
	return 0.0f;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	NoiseTool app(metric, filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
