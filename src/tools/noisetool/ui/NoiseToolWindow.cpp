/**
 * @file
 */

#include "NoiseToolWindow.h"
#include "../NoiseTool.h"
#include "noisedata/NoiseDataItemWidget.h"
#include "noise/Noise.h"
#include "noise/Simplex.h"
#include "ui/UIApp.h"
#include "core/Color.h"

#define IMAGE_PREFIX "2d"
#define GRAPH_PREFIX "graph"

NoiseToolWindow::NoiseToolWindow(NoiseTool* tool) :
		ui::Window(tool), _noiseTool(tool) {
	for (int i = 0; i < (int)NoiseType::Max; ++i) {
		addMenuItem(_noiseTypeSource, getNoiseTypeName((NoiseType)i));
	}
}

NoiseToolWindow::~NoiseToolWindow() {
	if (_noiseType != nullptr) {
		_noiseType->SetSource(nullptr);
	}
	delete[] _noiseBuffer;
	_noiseBuffer = nullptr;
	delete[] _graphBuffer;
	_graphBuffer = nullptr;
	delete[] _graphBufferBackground;
	_graphBufferBackground = nullptr;
}

bool NoiseToolWindow::init() {
	if (!loadResourceFile("ui/window/noisetool-main.tb.txt")) {
		Log::error("Failed to init the main window: Could not load the ui definition");
		return false;
	}
	_noiseType = getWidgetByType<tb::TBSelectDropdown>("type");
	if (_noiseType == nullptr) {
		Log::error("Failed to init the main window: No type widget found");
		return false;
	}
	_noiseType->SetSource(&_noiseTypeSource);

	_select = getWidgetByType<tb::TBSelectList>("list");
	if (_select == nullptr) {
		return false;
	}
	_select->SetSource(_noiseTool->noiseItemSource());
	_select->GetScrollContainer()->SetScrollMode(tb::SCROLL_MODE_X_AUTO_Y_AUTO);

	const tb::TBRect& rect = _select->GetPaddingRect();
	_noiseHeight = rect.h;
	_noiseWidth = rect.w - 60;
	const size_t noiseBufferSize = _noiseWidth * _noiseHeight * BPP;
	_noiseBuffer = new uint8_t[noiseBufferSize];
	const size_t graphBufferSize = _noiseWidth * _graphHeight * BPP;
	_graphBuffer = new uint8_t[graphBufferSize];
	_graphBufferBackground = new uint8_t[graphBufferSize];

	const int graphBufOffset = index(0, int(_graphHeight / 2));
	memset(&_graphBufferBackground[graphBufOffset], core::Color::getRGBA(core::Color::Gray), _noiseWidth * BPP);

	for (int i = 0; i < _graphHeight; ++i) {
		uint8_t* gbuf = &_graphBufferBackground[index(10, i)];
		*((uint32_t*)gbuf) = core::Color::getRGBA(core::Color::Gray);
	}

	return true;
}

float NoiseToolWindow::getNoise(int x, int y) {
	const auto millis = core::App::getInstance()->timeProvider()->currentTime();
	const NoiseType noiseType = _data.noiseType;
	const glm::vec2 position = (glm::vec2(_data.offset) + glm::vec2(x, y) * _data.frequency);
	switch (noiseType) {
	case NoiseType::simplexNoise:
		return noise::norm(noise::noise(position));
	case NoiseType::ridgedNoise:
		return noise::ridgedNoise(position);
	case NoiseType::flowNoise:
		return noise::norm(noise::flowNoise(position, millis));
	case NoiseType::fbm:
		return noise::norm(noise::fBm(position, _data.octaves, _data.lacunarity, _data.gain));
	case NoiseType::fbmCascade:
		return noise::norm(noise::fBm(noise::fBm(position * 3.0f)));
	case NoiseType::fbmAnalyticalDerivatives:
		return noise::norm(noise::fBm(noise::dfBm(position)));
	case NoiseType::flowNoiseFbm:
		return noise::norm(noise::flowNoise(position + noise::fBm(glm::vec3(position, millis * 0.1f)), millis));
	case NoiseType::ridgedMFTime:
		return noise::ridgedMF(glm::vec3(position, millis * 0.1f), _data.offset, _data.octaves, _data.lacunarity, _data.gain);
	case NoiseType::ridgedMF:
		return noise::ridgedMF(position, _data.offset, _data.octaves, _data.lacunarity, _data.gain);
	case NoiseType::ridgedMFCascade:
		return noise::ridgedMF(noise::ridgedMF(position));
	case NoiseType::ridgedMFScaled:
		return noise::ridgedMF(position * 0.25f, _data.offset, _data.octaves, _data.lacunarity, _data.gain);
	case NoiseType::iqNoise:
		return noise::norm(noise::iqMatfBm(position, _data.octaves, glm::mat2(2.3f, -1.5f, 1.5f, 2.3f), _data.gain));
	case NoiseType::iqNoiseScaled:
		return noise::norm(noise::iqMatfBm(position * 0.75f, _data.octaves, glm::mat2(-12.5f, -0.5f, 0.5f, -12.5f), _data.gain));
	case NoiseType::analyticalDerivatives:
		return (noise::dnoise(position * 5.0f).y + noise::dnoise(position * 5.0f).z) * 0.5f;
	case NoiseType::noiseCurlNoise:
		return noise::norm(noise::noise(position + glm::vec2(noise::curlNoise(position, millis).x)));
	case NoiseType::voronoi:
		return noise::norm(noise::voronoi(glm::dvec3(position, 0.0), true, _data.offset, _data.frequency, _data.octaves));
	case NoiseType::Max:
		break;
	}
	return 0.0f;
}

bool NoiseToolWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	const tb::TBID& id = ev.target->GetID();
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (id == TBIDC("ok")) {
			generateImage();
			return true;
		} else if (id == TBIDC("quit")) {
			Close();
			return true;
		}
	}
	if (ev.type == tb::EVENT_TYPE_CHANGED && id == TBIDC("filter") && _select != nullptr) {
		_select->SetFilter(ev.target->GetText());
		return true;
	}
	return Super::OnEvent(ev);
}

void NoiseToolWindow::generateImage() {
	const int type = getSelectedId("type");
	if (type < 0 || type >= (int)NoiseType::Max) {
		return;
	}

	_data.offset = getFloat("offset");
	_data.lacunarity = getFloat("lacunarity");
	_data.octaves = getInt("octaves");
	_data.gain = getFloat("gain");
	_data.frequency = getFloat("frequency");
	_data.noiseType = (NoiseType)type;

	const size_t noiseBufferSize = _noiseWidth * _noiseHeight * BPP;
	memset(_noiseBuffer, 255, noiseBufferSize);
	const size_t graphBufferSize = _noiseWidth * _graphHeight * BPP;
	memcpy(_graphBuffer, _graphBufferBackground, graphBufferSize);

	for (int y = 0; y < _noiseHeight; ++y) {
		for (int x = 0; x < _noiseWidth; ++x) {
			const float n = getNoise(x, y);
			const float cn = glm::clamp(n, 0.0f, 1.0f);
			const uint8_t c = cn * 255;
			uint8_t* buf = &_noiseBuffer[index(x, y)];
			memset(buf, c, BPP - 1);
			if (y == 0 && x < _noiseWidth) {
				uint8_t* gbuf = &_graphBuffer[index(x, (cn * _graphHeight) - 1)];
				*((uint32_t*)gbuf) = core::Color::getRGBA(core::Color::Red);
			}
		}
	}

	tb::TBStr idStr;
	idStr.SetFormatted(IMAGE_PREFIX "-%i-%f-%i-%f-%f-%f", (int)type, _data.offset, _data.octaves, _data.lacunarity, _data.gain, _data.frequency);

	tb::TBStr graphIdStr;
	graphIdStr.SetFormatted(GRAPH_PREFIX "-%i-%f-%i-%f-%f-%f", (int)type, _data.offset, _data.octaves, _data.lacunarity, _data.gain, _data.frequency);

	_data.noise = tb::g_image_manager->GetImage(idStr.CStr(), (uint32_t*)_noiseBuffer, _noiseWidth, _noiseHeight);
	_data.graph = tb::g_image_manager->GetImage(graphIdStr.CStr(), (uint32_t*)_graphBuffer, _noiseWidth, _graphHeight);

	_noiseTool->add(TBIDC(idStr), _data);
}

void NoiseToolWindow::OnDie() {
	if (_noiseType != nullptr) {
		_noiseType->SetSource(nullptr);
	}
	Super::OnDie();
	requestQuit();
}
