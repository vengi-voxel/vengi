/**
 * @file
 */

#include "NoiseToolWindow.h"

#include "noise/Noise.h"
#include "noise/Simplex.h"
#include "ui/UIApp.h"

NoiseToolWindow::NoiseToolWindow(ui::UIApp* tool) :
		ui::Window(tool) {
}

NoiseToolWindow::~NoiseToolWindow() {
	delete[] _autoBuffer;
}

bool NoiseToolWindow::init() {
	if (!loadResourceFile("ui/window/noisetool-main.tb.txt")) {
		Log::error("Failed to init the main window: Could not load the ui definition");
		return false;
	}

	_editorContainer = getWidget("editorcontainer");
	if (_editorContainer == nullptr) {
		Log::error("Failed to init the main window: No editorcontainer widget found");
		return false;
	}

	_imageLayout = getWidgetByType<tb::TBLayout>("imagelayout");
	if (_imageLayout == nullptr) {
		Log::error("Failed to init the main window: No imagelayout widget found");
		return false;
	}

	_graphBegin = getWidget("graphbegin");
	if (_graphBegin == nullptr) {
		Log::error("Failed to init the main window: No graphbegin widget found");
		return false;
	}

	_graphImage = getWidgetByType<ImageWidget>("graphimage");
	if (_graphImage == nullptr) {
		Log::error("Failed to init the main window: No graphimage widget found");
		return false;
	}

	return true;
}

void NoiseToolWindow::update(long dt) {
	_time += dt;
	_autoUpdate += dt;
	const long autoUpdate = 2500l;
	if (_autoUpdate < autoUpdate) {
		return;
	}

	_autoUpdate -= autoUpdate;
	const bool autoGenerate = isToggled("auto");
	if (autoGenerate) {
		generateImage();
	}
}

float NoiseToolWindow::getNoise(NoiseType noiseType, int x, int y) {
	const glm::vec2 position = (glm::vec2(_offset) + glm::vec2(x, y) * _frequency);
	switch (noiseType) {
	case NoiseType::simplexNoise:
		return noise::norm(noise::noise(position));
	case NoiseType::ridgedNoise:
		return noise::ridgedNoise(position);
	case NoiseType::flowNoise:
		return noise::norm(noise::flowNoise(position, _time));
	case NoiseType::fbm:
		return noise::norm(noise::fBm(position, _octaves, _lacunarity, _gain));
	case NoiseType::fbmCascade:
		return noise::norm(noise::fBm(noise::fBm(position * 3.0f)));
	case NoiseType::fbmAnalyticalDerivatives:
		return noise::norm(noise::fBm(noise::dfBm(position)));
	case NoiseType::flowNoiseFbm:
		return noise::norm(noise::flowNoise(position + noise::fBm(glm::vec3(position, _time * 0.1f)), _time));
	case NoiseType::ridgedMFTime:
		return noise::ridgedMF(glm::vec3(position, _time * 0.1f), _offset, _octaves, _lacunarity, _gain);
	case NoiseType::ridgedMF:
		return noise::ridgedMF(position, _offset, _octaves, _lacunarity, _gain);
	case NoiseType::ridgedMFCascade:
		return noise::ridgedMF(noise::ridgedMF(position));
	case NoiseType::ridgedMFScaled:
		return noise::ridgedMF(position * 0.25f, _offset, _octaves, _lacunarity, _gain);
	case NoiseType::iqNoise:
		return noise::norm(noise::iqMatfBm(position, _octaves, glm::mat2(2.3f, -1.5f, 1.5f, 2.3f), _gain));
	case NoiseType::iqNoiseScaled:
		return noise::norm(noise::iqMatfBm(position * 0.75f, _octaves, glm::mat2(-12.5f, -0.5f, 0.5f, -12.5f), _gain));
	case NoiseType::analyticalDerivatives:
		return (noise::dnoise(position * 5.0f).y + noise::dnoise(position * 5.0f).z) * 0.5f;
	case NoiseType::noiseCurlNoise:
		return noise::norm(noise::noise(position + glm::vec2(noise::curlNoise(position, _time).x)));
	case NoiseType::Max:
		break;
	}
	return 0.0f;
}

void NoiseToolWindow::makeSingle2DNoise(bool append, NoiseType noiseType) {
	// TODO: this 30 is the scrollbar - how to do this properly?
	const int scrollbarWidth = 30;

	const tb::TBRect& noiseRect = _editorContainer->GetPaddingRect();
	const int noiseHeight = noiseRect.h;
	const int noiseWidth = noiseRect.w - scrollbarWidth;
	const int components = 4;
	uint8_t noiseBuffer[noiseWidth * noiseHeight * components];
	memset(noiseBuffer, 255, sizeof(noiseBuffer));

	tb::TBStr idStr;
	idStr.SetFormatted("2d-%i-%f-%i-%f-%f-%f", (int)noiseType, _offset, _octaves, _lacunarity, _gain, _frequency);
	cleanup(idStr);

	const tb::TBRect& graphRect = _graphBegin->GetPaddingRect();
	const int graphHeight = graphRect.h;
	const int graphWidth = noiseWidth - graphRect.w;
	uint8_t graphBuffer[graphWidth * graphHeight * components];
	memset(graphBuffer, 0, sizeof(graphBuffer));

	for (int y = 0; y < noiseHeight; ++y) {
		for (int x = 0; x < noiseWidth; ++x) {
			const float n = getNoise(noiseType, x, y);
			const uint8_t c = glm::clamp(n, 0.0f, 1.0f) * 255;
			uint8_t* buf = &noiseBuffer[x * components + y * noiseWidth * components];
			const int j = components == 4 ? 3 : 4;
			for (int i = 0; i < j; ++i) {
				buf[i] = c;
			}
			if (y == 0) {
				const float graphN = glm::clamp(n, 0.0f, 1.0f);
				const int graphY = graphN * graphHeight;
				const int graphBufOffset = x * components + graphY * graphWidth * components;
				uint8_t* gbuf = &graphBuffer[graphBufOffset];
				gbuf[3] = 255;
			}
		}
	}

	_noiseType = noiseType;
	_dirtyParameters = false;

	addImage(idStr, append, noiseBuffer, noiseWidth, noiseHeight);
	addGraph("graph", graphBuffer, graphWidth, graphHeight);
}

void NoiseToolWindow::fillBuffer(NoiseType noiseType, int width, int height, int components, int cols, int rows, int widgetWidth) {
	for (int y = 0; y < height; ++y) {
		const int xStart = ((int)noiseType % cols) * width;
		const int yStart = ((int)noiseType / cols) * height + y;
		const int bufOffset = xStart * components + yStart * widgetWidth * components;
		for (int x = 0; x < width; ++x) {
			uint8_t* buf = &_autoBuffer[bufOffset + x * components];
			const float n = getNoise(noiseType, x, y);
			const uint8_t c = glm::clamp(n, 0.0f, 1.0f) * 255;
			const int j = components == 4 ? 3 : 4;
			for (int i = 0; i < j; ++i) {
				buf[i] = c;
			}
		}
	}
}

void NoiseToolWindow::allInOne2DNoise() {
	const tb::TBRect& rect = _editorContainer->GetPaddingRect();
	const int widgetHeight = rect.h;
	// TODO: this 30 is the scrollbar - how to do this properly?
	const int widgetWidth = rect.w - 30;
	const int components = 4;
	if (widgetHeight != _autoHeight || widgetWidth != _autoWidth) {
		delete[] _autoBuffer;
		_dirtyParameters = true;
		_autoBuffer = new uint8_t[widgetWidth * widgetHeight * components];
		memset(_autoBuffer, 255, sizeof(uint8_t) * widgetWidth * widgetHeight * components);
		_autoWidth = widgetWidth;
		_autoHeight = widgetHeight;
	}
	tb::TBStr idStr("2d-auto");
	cleanup(idStr);
	const int cols = 8;
	const int rows = 2;
	const int width = widgetWidth / cols;
	const int height = widgetHeight / rows;
	for (int noiseType = 0; noiseType < (int)NoiseType::Max; ++noiseType) {
		if (!_dirtyParameters) {
			if (noiseType != (int)NoiseType::flowNoise
			 && noiseType != (int)NoiseType::flowNoiseFbm
			 && noiseType != (int)NoiseType::ridgedMFTime
			 && noiseType != (int)NoiseType::noiseCurlNoise) {
				// only recreate time based values - the others wouldn't change
				continue;
			}
		}
		fillBuffer((NoiseType)noiseType, width, height, components, cols, rows, widgetWidth);
	}
	_dirtyParameters = false;
	addImage(idStr, false, _autoBuffer, widgetWidth, widgetHeight);
}

void NoiseToolWindow::cleanup(const tb::TBStr& idStr) {
	tb::TBBitmapFragmentManager *fragMgr = tb::g_tb_skin->GetFragmentManager();
	const tb::TBID id(idStr.CStr());
	tb::TBBitmapFragment *existingFragment = fragMgr->GetFragment(id);
	if (existingFragment != nullptr) {
		fragMgr->FreeFragment(existingFragment);
	}
}

void NoiseToolWindow::addGraph(const tb::TBStr& idStr, uint8_t* buffer, int width, int height) {
	const tb::TBImage& image = tb::g_image_manager->GetImage(idStr.CStr(), (uint32_t*)buffer, width, height);
	// TODO: update doesn't work here - we have to change the id
	_graphImage->SetImage(image);
}

void NoiseToolWindow::addImage(const tb::TBStr& idStr, bool append, uint8_t* buffer, int width, int height) {
	if (!append) {
		_imageLayout->DeleteAllChildren();
	}
	tb::TBImageWidget* imageWidget = new tb::TBImageWidget();
	tb::TBTextField* caption = new tb::TBTextField();
	caption->SetText(idStr.CStr());
	caption->SetGravity(tb::WIDGET_GRAVITY_BOTTOM | tb::WIDGET_GRAVITY_LEFT_RIGHT);
	caption->SetSkinBg(tb::TBID("image_caption"));
	imageWidget->AddChild(caption, tb::WIDGET_Z_BOTTOM);
	imageWidget->OnInflateChild(caption);

	tb::TBButton* removeButton = new tb::TBButton();
	removeButton->SetID(tb::TBID("remove"));
	removeButton->SetSkinBg(tb::TBID("button_remove"));
	removeButton->SetGravity(tb::WIDGET_GRAVITY_RIGHT);
	imageWidget->AddChild(removeButton, tb::WIDGET_Z_BOTTOM);
	imageWidget->OnInflateChild(removeButton);

	const tb::TBImage& image = tb::g_image_manager->GetImage(idStr.CStr(), (uint32_t*)buffer, width, height);
	imageWidget->SetImage(image);
	_imageLayout->AddChild(imageWidget, tb::WIDGET_Z_BOTTOM);
	_imageLayout->OnInflateChild(imageWidget);
}

bool NoiseToolWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->GetID() == TBIDC("remove")) {
			tb::TBWidget *image = ev.target->GetParent();
			removeImage(image);
			return true;
		} else if (ev.target->GetID() == TBIDC("ok")) {
			generateImage();
			return true;
		} else if (ev.target->GetID() == TBIDC("quit")) {
			Close();
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_DOWN) {
		if (ev.special_key == tb::TB_KEY_DELETE) {
			tb::TBWidget *lastImage =_imageLayout->GetFirstChild();
			if (lastImage != nullptr) {
				removeImage(lastImage);
				return true;
			}
		} else if (ev.special_key == tb::TB_KEY_ENTER) {
			generateImage();
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_SHORTCUT) {
		if (ev.ref_id == TBIDC("new")) {
			//removeImage();
			generateImage();
			return true;
		} else if (ev.target->GetID() == TBIDC("cut")) {
			//removeImage();
			return true;
		}
	}
	return ui::Window::OnEvent(ev);
}

void NoiseToolWindow::generateImage() {
	const bool append = isToggled("append");
	const int type = getSelectedId("type");
	const bool allInOne = isToggled("allinone");

	const float offset = getFloat("offset");
	const float lacunarity = getFloat("lacunarity");
	const int octaves = getInt("octaves");
	const float gain = getFloat("gain");
	const float frequency = getFloat("frequency");

	if (!glm::epsilonEqual(offset, _offset, glm::epsilon<float>()) ||
		!glm::epsilonEqual(lacunarity, _lacunarity, glm::epsilon<float>()) ||
		!glm::epsilonEqual(frequency, _frequency, glm::epsilon<float>()) ||
		!glm::epsilonEqual(gain, _gain, glm::epsilon<float>()) ||
		octaves != _octaves) {
		_dirtyParameters = true;
	}

	_offset = offset;
	_lacunarity = lacunarity;
	_octaves = octaves;
	_gain = gain;
	_frequency = frequency;

	if (allInOne) {
		allInOne2DNoise();
	} else {
		if (type < 0 || type >= (int)NoiseType::Max) {
			return;
		}
		const NoiseType noiseType = (NoiseType)type;
		_dirtyParameters = _dirtyParameters || _noiseType != noiseType;
		if (!_dirtyParameters) {
			if (noiseType != NoiseType::flowNoise
			 && noiseType != NoiseType::flowNoiseFbm
			 && noiseType != NoiseType::ridgedMFTime
			 && noiseType != NoiseType::noiseCurlNoise) {
				// only recreate time based values - the others wouldn't change
				return;
			}
		}
		makeSingle2DNoise(append, noiseType);
	}
}

void NoiseToolWindow::removeImage(tb::TBWidget *image) {
	image->GetParent()->RemoveChild(image);
	delete image;
}

void NoiseToolWindow::OnDie() {
	ui::Window::OnDie();
	requestQuit();
}
