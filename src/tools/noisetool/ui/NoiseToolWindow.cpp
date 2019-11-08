/**
 * @file
 */

#include "NoiseToolWindow.h"
#include "../NoiseTool.h"
#include "noisedata/NoiseDataItemWidget.h"
#include "noise/Noise.h"
#include "ui/turbobadger/UIApp.h"
#include "core/Color.h"
#include "math/Rect.h"
#include "NoiseDataNodeWindow.h"
#include "noise/PoissonDiskDistribution.h"
#include "noise/Simplex.h"

#define IMAGE_PREFIX "2d"
#define GRAPH_PREFIX "graph"

NoiseToolWindow::NoiseToolWindow(NoiseTool* tool) :
		Super(tool), _noiseTool(tool) {
	for (int i = 0; i < (int)NoiseType::Max; ++i) {
		addStringItem(_noiseTypeSource, getNoiseTypeName((NoiseType)i));
	}
}

NoiseToolWindow::~NoiseToolWindow() {
	if (_noiseType != nullptr) {
		_noiseType->setSource(nullptr);
	}
	if (_select != nullptr) {
		_select->setSource(nullptr);
	}
	delete[] _graphBufferBackground;
	_graphBufferBackground = nullptr;
	_noise.shutdown();
}

bool NoiseToolWindow::init() {
	if (!_noise.init()) {
		return false;
	}
	if (!loadResourceFile("ui/window/noisetool-main.tb.txt")) {
		Log::error("Failed to init the main window: Could not load the ui definition");
		return false;
	}
	_noiseType = getWidgetByType<tb::TBSelectDropdown>("type");
	if (_noiseType == nullptr) {
		Log::error("Failed to init the main window: No type widget found");
		return false;
	}
	_noiseType->setSource(&_noiseTypeSource);

	_select = getWidgetByType<tb::TBSelectList>("list");
	if (_select == nullptr) {
		return false;
	}
	_select->setSource(_noiseTool->noiseItemSource());
	_select->getScrollContainer()->setScrollMode(tb::SCROLL_MODE_X_AUTO_Y_AUTO);

	const tb::TBRect& rect = _select->getPaddingRect();
	_noiseHeight = rect.h;
	_noiseWidth = rect.w - 60;
	const size_t graphBufferSize = _noiseWidth * _graphHeight * BPP;

	_graphBufferBackground = new uint8_t[graphBufferSize];
	const int graphBufOffset = index(0, int(_graphHeight / 2));
	memset(&_graphBufferBackground[graphBufOffset], core::Color::getRGBA(core::Color::Gray), _noiseWidth * BPP);

	for (int i = 0; i < _graphHeight; ++i) {
		uint8_t* gbuf = &_graphBufferBackground[index(10, i)];
		*((uint32_t*)gbuf) = core::Color::getRGBA(core::Color::Gray);
	}

	return true;
}

void NoiseToolWindow::updateForNoiseType(NoiseType type) {
	setActive("enabledistance", type == NoiseType::voronoi);
	setActive("seed", type == NoiseType::voronoi);
	setActive("separation", type == NoiseType::poissonDiskDistribution);
	setActive("lacunarity", type == NoiseType::fbm || type == NoiseType::ridgedMFTime || type == NoiseType::ridgedMF || type == NoiseType::worleyNoiseFbm || type == NoiseType::swissTurbulence);
	setActive("octaves", type == NoiseType::fbm || type == NoiseType::ridgedMFTime || type == NoiseType::ridgedMF || type == NoiseType::iqNoise || type == NoiseType::worleyNoiseFbm || type == NoiseType::swissTurbulence || type == NoiseType::jordanTurbulence);
	setActive("gain", type == NoiseType::fbm || type == NoiseType::ridgedMFTime || type == NoiseType::ridgedMF || type == NoiseType::iqNoise || type == NoiseType::worleyNoiseFbm || type == NoiseType::swissTurbulence);
	setActive("ridgedoffset", type == NoiseType::ridgedMFTime || type == NoiseType::ridgedMF);
	setActive("offset", true);
	setActive("frequency", true);
}

float NoiseToolWindow::getNoise(int x, int y, NoiseData data) {
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

bool NoiseToolWindow::onEvent(const tb::TBWidgetEvent &ev) {
	const tb::TBID& id = ev.target->getID();
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (id == TBIDC("ok")) {
			generateImage();
			return true;
		} else if (id == TBIDC("all")) {
			generateAll();
			return true;
		} else if (id == TBIDC("quit")) {
			close();
			return true;
		} else if (id == TBIDC("nodes")) {
			NoiseDataNodeWindow* window = new NoiseDataNodeWindow(_noiseTool);
			window->init();
			return true;
		}
	}

	if (ev.type == tb::EVENT_TYPE_CHANGED) {
		if (id == TBIDC("filter") && _select != nullptr) {
			_select->setFilter(ev.target->getText());
			return true;
		} else if (id == TBIDC("type")) {
			const int type = getSelectedId("type");
			if (type >= 0 && type < (int)NoiseType::Max) {
				NoiseType noiseType = (NoiseType)type;
				updateForNoiseType(noiseType);
			}
			return true;
		}
	}
	return Super::onEvent(ev);
}

void NoiseToolWindow::generateImage() {
	const int type = getSelectedId("type");
	if (type < 0 || type >= (int)NoiseType::Max) {
		return;
	}

	generateImage((NoiseType)type);
}

void NoiseToolWindow::generateAll() {
	for (int i = 0; i < (int)NoiseType::Max; ++i) {
		NoiseType type = (NoiseType)i;
		generateImage(type);
	}
}

void NoiseToolWindow::generateImage(NoiseType type) {
	core_trace_scoped(GenerateImage);
	Log::info("Generate noise for %s", getNoiseTypeName(type));
	NoiseData data;
	data.enableDistance = isToggled("enabledistance");
	data.separation = getFloat("separation");
	data.seed = getInt("seed");
	data.offset = getFloat("offset");
	data.lacunarity = getFloat("lacunarity");
	data.octaves = getInt("octaves");
	data.gain = getFloat("gain");
	data.frequency = getFloat("frequency");
	data.ridgedOffset = getFloat("ridgedoffset");
	data.noiseType = type;

	_noiseTool->threadPool().enqueue([this, data] () {
		const size_t noiseBufferSize = _noiseWidth * _noiseHeight * BPP;
		const size_t graphBufferSize = _noiseWidth * _graphHeight * BPP;
		QueueData qd;
		qd.data = data;;
		qd.data.millis = _noiseTool->timeProvider()->systemMillis();
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
					memset(buf, c, BPP - 1);
					if (y == 0 && x < _noiseWidth) {
						const int gy = h - (cn * h);
						const int idx = index(x, gy);
						uint8_t* gbuf = &graphBuffer[idx];
						*((uint32_t*)gbuf) = core::Color::getRGBA(core::Color::Red);
					}
				}
			}
		}

		qd.data.endmillis = _noiseTool->timeProvider()->systemMillis();
		this->_queue.push(qd);
	});
}

void NoiseToolWindow::update() {
	QueueData qd;
	if (!_queue.pop(qd)) {
		return;
	}
	NoiseData& data = qd.data;

	const std::string& buf = core::string::format("-%f-%i-%f-%i-%f-%i-%f-%f-%f",
			data.ridgedOffset, (int)data.noiseType, data.separation, (int)data.enableDistance,
			data.offset, data.octaves, data.lacunarity, data.gain, data.frequency);

	const std::string imageBuf = IMAGE_PREFIX + buf;
	data.noise = tb::g_image_manager->getImage(imageBuf.c_str(), (uint32_t*)qd.noiseBuffer, _noiseWidth, _noiseHeight);

	const std::string graphBuf = GRAPH_PREFIX + buf;
	data.graph = tb::g_image_manager->getImage(graphBuf.c_str(), (uint32_t*)qd.graphBuffer, _noiseWidth, _graphHeight);

	_noiseTool->add(TBIDC(graphBuf.c_str()), data);

	const int n = _select->getSource()->getNumItems();
	_select->setValue(n - 1);
	Log::info("Generating noise for %s took %" SDL_PRIu64 "ms", getNoiseTypeName(data.noiseType), data.endmillis - data.millis);
	delete [] qd.noiseBuffer;
	delete [] qd.graphBuffer;
}

void NoiseToolWindow::onDie() {
	Super::onDie();
	requestQuit();
}
