/**
 * @file
 */

#include "NoiseToolWindow.h"
#include "../NoiseTool.h"
#include "noise/Noise.h"
#include "noise/Simplex.h"
#include "ui/UIApp.h"

static const char *NoiseTypeStr[] = {
	"simplex noise",
	"ridged noise",
	"flow noise (rot. gradients)",
	"fractal brownian motion sum",
	"fbm",
	"fbm cascade",
	"fbm analytical derivatives",
	"flow noise fbm (time)",
	"ridged multi fractal",
	"ridged multi fractal cascade",
	"ridged multi fractal scaled",
	"iq noise",
	"iq noise scaled",
	"analytical derivatives",
	"noise curl noise (time)",
	"voronoi"
};
static_assert((int)SDL_arraysize(NoiseTypeStr) == (int)NoiseType::Max, "String array size doesn't match noise types");

#define IMAGE_PREFIX "2d"
#define GRAPH_PREFIX "graph"

NoiseToolWindow::NoiseToolWindow(NoiseTool* tool) :
		ui::Window(tool), _noiseTool(tool) {
	for (int i = 0; i < (int)SDL_arraysize(NoiseTypeStr); ++i) {
		addMenuItem(_noiseTypes, NoiseTypeStr[i]);
	}
}

NoiseToolWindow::~NoiseToolWindow() {
	delete[] _autoBuffer;
	tb::TBSelectDropdown* noiseTypes = getWidgetByType<tb::TBSelectDropdown>("type");
	if (noiseTypes != nullptr) {
		noiseTypes->SetSource(nullptr);
	}
}

bool NoiseToolWindow::init() {
	if (!loadResourceFile("ui/window/noisetool-main.tb.txt")) {
		Log::error("Failed to init the main window: Could not load the ui definition");
		return false;
	}

	tb::TBSelectDropdown* noiseTypes = getWidgetByType<tb::TBSelectDropdown>("type");
	if (noiseTypes == nullptr) {
		Log::error("Failed to init the main window: No noisetypes widget found");
		return false;
	}
	noiseTypes->SetSource(&_noiseTypes);

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

	_imageScrollContainer = getWidgetByType<tb::TBScrollContainer>("imagescroll");
	if (_imageScrollContainer == nullptr) {
		Log::error("Failed to init the main window: No imagescroll widget found");
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

std::string NoiseToolWindow::getCaption(tb::TBWidget* widget) const {
	if (widget == nullptr) {
		return "empty";
	}
	tb::TBTextField* caption = widget->GetWidgetByIDAndType<tb::TBTextField>(TBIDC("caption"));
	if (caption == nullptr) {
		return "empty";
	}
	const std::string str = caption->GetText().CStr();
	return str;
}

void NoiseToolWindow::activateGraph() {
	const TBWidget::ScrollInfo& scrollInfo = _imageScrollContainer->GetScrollInfo();
	tb::TBWidget* widget = _imageLayout->GetWidgetAt(scrollInfo.x, scrollInfo.y, false);
	const std::string& caption = getCaption(widget);
	if (_lastActiveImage == caption) {
		return;
	}
	_lastActiveImage = caption;
	Log::debug("Active image: %s", _lastActiveImage.c_str());
	const std::string& graphId = getGraphName(_lastActiveImage.c_str());
	Log::debug("Looking for graph for: %s", graphId.c_str());
	static int32_t empty = 0x000000;
	addGraph(graphId, (uint8_t*)&empty, 1, 1);
}

std::string NoiseToolWindow::getGraphName(const char* idStr) const {
	tb::TBStr graphStr;
	graphStr.SetFormatted(GRAPH_PREFIX "%s", idStr + strlen(IMAGE_PREFIX));
	const std::string graphId = graphStr.CStr();
	return graphId;
}

void NoiseToolWindow::update(long dt) {
	_time += dt;
	_autoUpdate += dt;

	activateGraph();

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
	const glm::vec2 position = (glm::vec2(_data.offset) + glm::vec2(x, y) * _data.frequency);
	switch (noiseType) {
	case NoiseType::simplexNoise:
		return noise::norm(noise::noise(position));
	case NoiseType::ridgedNoise:
		return noise::ridgedNoise(position);
	case NoiseType::flowNoise:
		return noise::norm(noise::flowNoise(position, _time));
	case NoiseType::fbm:
		return noise::norm(noise::fBm(position, _data.octaves, _data.lacunarity, _data.gain));
	case NoiseType::fbmCascade:
		return noise::norm(noise::fBm(noise::fBm(position * 3.0f)));
	case NoiseType::fbmAnalyticalDerivatives:
		return noise::norm(noise::fBm(noise::dfBm(position)));
	case NoiseType::flowNoiseFbm:
		return noise::norm(noise::flowNoise(position + noise::fBm(glm::vec3(position, _time * 0.1f)), _time));
	case NoiseType::ridgedMFTime:
		return noise::ridgedMF(glm::vec3(position, _time * 0.1f), _data.offset, _data.octaves, _data.lacunarity, _data.gain);
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
		return noise::norm(noise::noise(position + glm::vec2(noise::curlNoise(position, _time).x)));
	case NoiseType::voronoi:
		return noise::norm(noise::voronoi(glm::dvec3(position, 0.0), true, _data.offset, _data.frequency, _data.octaves));
	case NoiseType::Max:
		break;
	}
	return 0.0f;
}

void NoiseToolWindow::makeSingle2DNoise(bool append, NoiseType noiseType) {
	const tb::TBRect& noiseRect = _editorContainer->GetPaddingRect();
	const int noiseHeight = noiseRect.h;
	const int noiseWidth = noiseRect.w;
	const int components = 4;
	uint8_t noiseBuffer[noiseWidth * noiseHeight * components];
	memset(noiseBuffer, 255, sizeof(noiseBuffer));

	tb::TBStr idStr;
	idStr.SetFormatted(IMAGE_PREFIX "-%i-%f-%i-%f-%f-%f", (int)noiseType, _data.offset, _data.octaves, _data.lacunarity, _data.gain, _data.frequency);
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
			if (y == 0 && x < graphWidth) {
				const float graphN = glm::clamp(n, 0.0f, 1.0f);
				const int graphY = graphN * graphHeight;
				const int graphBufOffset = x * components + graphY * graphWidth * components;
				uint8_t* gbuf = &graphBuffer[graphBufOffset];
				gbuf[3] = 255;
			}
		}
	}

	_data.noiseType = noiseType;
	_dirtyParameters = false;

	addImage(idStr, append, noiseBuffer, noiseWidth, noiseHeight);

	const std::string& graphName = getGraphName(idStr);
	_graphs.insert(std::make_pair(graphName, tb::g_image_manager->GetImage(graphName.c_str(), (uint32_t*)graphBuffer, graphWidth, graphHeight)));
	_noiseTool->add(TBIDC(idStr), _data);
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
	tb::TBStr idStr(IMAGE_PREFIX "-auto");
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

	const std::string& graphId = getGraphName(idStr.CStr());
	auto i = _graphs.find(graphId);
	if (i != _graphs.end()) {
		_graphs.erase(i);
		_lastActiveImage = "";
	}

	_noiseTool->remove(id);
}

void NoiseToolWindow::addGraph(const std::string& idStr, uint8_t* buffer, int width, int height) {
	const tb::TBImage& image = tb::g_image_manager->GetImage(idStr.c_str(), (uint32_t*)buffer, width, height);
	_graphImage->SetImage(image);
}

void NoiseToolWindow::addImage(const tb::TBStr& idStr, bool append, uint8_t* buffer, int width, int height) {
	if (!append) {
		_imageLayout->DeleteAllChildren();
	}

	tb::TBImageWidget* imageWidget = new tb::TBImageWidget();
	tb::TBTextField* caption = new tb::TBTextField();
	caption->SetText(idStr.CStr());
	caption->SetID(TBIDC("caption"));
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
		} else if (ev.special_key == tb::TB_KEY_LEFT || ev.special_key == tb::TB_KEY_UP || ev.special_key == tb::TB_KEY_PAGE_UP) {
			_imageScrollContainer->ScrollBy(0, -_editorContainer->GetPaddingRect().h);
		} else if (ev.special_key == tb::TB_KEY_RIGHT || ev.special_key == tb::TB_KEY_DOWN || ev.special_key == tb::TB_KEY_PAGE_DOWN) {
			_imageScrollContainer->ScrollBy(0, _editorContainer->GetPaddingRect().h);
		} else if (ev.special_key == tb::TB_KEY_HOME) {
			_imageScrollContainer->ScrollTo(0, 0);
		} else if (ev.special_key == tb::TB_KEY_END) {
			_imageScrollContainer->ScrollTo(0, std::numeric_limits<int>::max());
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

	if (!glm::epsilonEqual(offset, _data.offset, glm::epsilon<float>()) ||
		!glm::epsilonEqual(lacunarity, _data.lacunarity, glm::epsilon<float>()) ||
		!glm::epsilonEqual(frequency, _data.frequency, glm::epsilon<float>()) ||
		!glm::epsilonEqual(gain, _data.gain, glm::epsilon<float>()) ||
		octaves != _data.octaves) {
		_dirtyParameters = true;
	}

	_data.offset = offset;
	_data.lacunarity = lacunarity;
	_data.octaves = octaves;
	_data.gain = gain;
	_data.frequency = frequency;

	if (allInOne) {
		allInOne2DNoise();
	} else {
		if (type < 0 || type >= (int)NoiseType::Max) {
			return;
		}
		const NoiseType noiseType = (NoiseType)type;
		_dirtyParameters = _dirtyParameters || _data.noiseType != noiseType;
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
	const std::string& caption = getCaption(image);
	const std::string& graphId = getGraphName(caption.c_str());
	auto i = _graphs.find(graphId);
	if (i != _graphs.end()) {
		_graphs.erase(i);
	}

	_noiseTool->remove(tb::TBID(caption.c_str()));

	image->GetParent()->RemoveChild(image);
	_lastActiveImage = "";
	_dirtyParameters = true;
	delete image;
}

void NoiseToolWindow::OnDie() {
	tb::TBSelectDropdown* noiseTypes = getWidgetByType<tb::TBSelectDropdown>("type");
	if (noiseTypes != nullptr) {
		noiseTypes->SetSource(nullptr);
	}
	ui::Window::OnDie();
	requestQuit();
}
