/**
 * @file
 */

#include "WindowedApp.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "io/Filesystem.h"
#include "core/Singleton.h"
#include "core/Var.h"
#include "Renderer.h"
#include "gl/GLVersion.h"
#include "Shader.h"
#include "core/Array.h"
#include "core/Color.h"
#include "core/command/Command.h"
#include "core/Singleton.h"
#include "ShaderManager.h"
#include "util/KeybindingHandler.h"
#include "util/KeybindingParser.h"
#include "util/CustomButtonNames.h"

namespace video {

namespace {
inline void checkSDLError(const char *file, unsigned int line, const char *function) {
	const char *error = SDL_GetError();
	if (*error != '\0') {
		Log::error("%s (%s:%i => %s)", error, file, line, function);
		SDL_ClearError();
	} else {
		Log::error("unknown error (%s:%i => %s)", file, line, function);
	}
}
#define sdlCheckError() checkSDLError(__FILE__, __LINE__, SDL_FUNCTION)
}

WindowedApp::WindowedApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider), _frameBufferDimension(-1), _mousePos(-1), _mouseRelativePos(-1) {
}

WindowedApp::~WindowedApp() {
}

void WindowedApp::onAfterRunning() {
	core_trace_scoped(WindowedAppAfterRunning);
	video::endFrame(_window);
}

core::AppState WindowedApp::onRunning() {
	core_trace_scoped(WindowedAppOnRunning);
	Super::onRunning();

	SDL_GetMouseState(&_mousePos.x, &_mousePos.y);
	SDL_GetRelativeMouseState(&_mouseRelativePos.x, &_mouseRelativePos.y);
	SDL_Event event;
	bool quit = false;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			// continue to handle any other following event
			quit = true;
			break;
		case SDL_WINDOWEVENT:
			// we must be the first to handle this - but others should get their chance, too
			if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				const int w = event.window.data1;
				const int h = event.window.data2;
				int frameBufferWidth, frameBufferHeight;
				SDL_GL_GetDrawableSize(_window, &frameBufferWidth, &frameBufferHeight);
				_aspect = frameBufferWidth / static_cast<float>(frameBufferHeight);
				_frameBufferDimension = glm::ivec2(frameBufferWidth, frameBufferHeight);
				_windowDimension = glm::ivec2(w, h);
				const float scaleFactor = _frameBufferDimension.x / (float)_windowDimension.x;
				video::resize(w, h, scaleFactor);
				video::viewport(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
			}
			// fallthrough
		default: {
			core_trace_scoped(WindowedAppEventHandler);
			const bool running = core::Singleton<io::EventHandler>::getInstance().handleEvent(event);
			if (!running) {
				quit = true;
			}
			break;
		}
		}
	}

	if (quit) {
		return core::AppState::Cleanup;
	}

	core_trace_scoped(WindowedAppStartFrame);
	video::startFrame(_window, _rendererContext);
	video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);
	core::Singleton<ShaderManager>::getInstance().update();

	++_frameCounter;

	double time = _now;
	if (time > _frameCounterResetTime + 1000) {
		_fps = (int) round((_frameCounter / (time - _frameCounterResetTime)) * 1000);
		_frameCounterResetTime = time;
		_frameCounter = 0;
	}

	return core::AppState::Running;
}

bool WindowedApp::onKeyRelease(int32_t key, int16_t modifier) {
	return handleKeyRelease(key, modifier);
}

bool WindowedApp::handleKeyRelease(int32_t key, int16_t /*modifier*/) {
	// don't use modifier here - this is the modifier that was released. But
	// we need the currently pressed modifier mask
	return _keybindingHandler.execute(key, SDL_GetModState(), false, _now);
}

bool WindowedApp::handleKeyPress(int32_t key, int16_t modifier) {
	return _keybindingHandler.execute(key, modifier, true, _now);
}

bool WindowedApp::onMouseWheel(int32_t x, int32_t y) {
	const int32_t key = y < 0 ? util::button::CUSTOM_SDLK_MOUSE_WHEEL_UP : util::button::CUSTOM_SDLK_MOUSE_WHEEL_DOWN;
	const int16_t mod = SDL_GetModState();
	if (handleKeyPress(key, mod)) {
		handleKeyRelease(key, mod);
		return true;
	}
	return false;
}

void WindowedApp::onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	if (clicks > 1) {
		return;
	}
	handleKeyPress(CUSTOM_SDL_KEYCODE(button), SDL_GetModState());
}

void WindowedApp::onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	handleKeyRelease(CUSTOM_SDL_KEYCODE(button), SDL_GetModState());
}

bool WindowedApp::onKeyPress(int32_t key, int16_t modifier) {
	if (modifier & KMOD_LALT) {
		if (key == SDLK_RETURN) {
			const int flags = SDL_GetWindowFlags(_window);
			if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
				SDL_SetWindowFullscreen(_window, 0);
			} else {
				SDL_SetWindowFullscreen(_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
			}
			return true;
		}
	}
	handleKeyPress(key, modifier);
	return true;
}

std::string WindowedApp::getKeyBindingsString(const char *cmd) const {
	return _keybindingHandler.getKeyBindingsString(cmd);
}

core::AppState WindowedApp::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		sdlCheckError();
		return core::AppState::InitFailure;
	}

	SDL_EventState(SDL_MOUSEMOTION, SDL_DISABLE);
	SDL_StopTextInput();

	if (!_keybindingHandler.init()) {
		Log::error("Failed to initialize the key binding hendler");
		return core::AppState::InitFailure;
	}
	if (!_keybindingHandler.load("keybindings.cfg")) {
		Log::warn("failed to init the global keybindings");
	}
	_keybindingHandler.load(_appname + "-keybindings.cfg");

	core::Singleton<io::EventHandler>::getInstance().registerObserver(this);

	Log::info("CPU count: %d", SDL_GetCPUCount());
	Log::info("CacheLine size: %d", SDL_GetCPUCacheLineSize());
	Log::info("RDTSC: %d", SDL_HasRDTSC());
	Log::info("Altivec: %d", SDL_HasAltiVec());
	Log::info("MMX: %d", SDL_HasMMX());
	Log::info("3DNow: %d", SDL_Has3DNow());
	Log::info("SSE: %d", SDL_HasSSE());
	Log::info("SSE2: %d", SDL_HasSSE2());
	Log::info("SSE3: %d", SDL_HasSSE3());
	Log::info("SSE4.1: %d", SDL_HasSSE41());
	Log::info("SSE4.2: %d", SDL_HasSSE42());
	Log::info("AVX: %d", SDL_HasAVX());
	Log::info("AVX2: %d", SDL_HasAVX2());
#if SDL_VERSION_ATLEAST(2, 0, 9)
	Log::info("NEON: %d", SDL_HasNEON());
#endif
	Log::info("RAM: %d MB", SDL_GetSystemRAM());

	SDL_DisplayMode displayMode;
	const int numDisplays = core_max(0, SDL_GetNumVideoDisplays());
	const int displayIndex = glm::clamp(core::Var::getSafe(cfg::ClientWindowDisplay)->intVal(), 0, numDisplays);
	SDL_GetDesktopDisplayMode(displayIndex, &displayMode);

	for (int i = 0; i < numDisplays; ++i) {
		SDL_Rect dr;
		if (SDL_GetDisplayBounds(i, &dr) == -1) {
			continue;
		}
		float ddpi = -1.0f;
		float hdpi = -1.0f;
		float vdpi = -1.0f;
		if (SDL_GetDisplayDPI(i, &ddpi, &hdpi, &vdpi) == 0) {
#ifdef _APPLE_
			const float baseDpi = 72.0f;
#else
			const float baseDpi = 96.0f;
#endif
			_dpiFactor = ddpi / baseDpi;
			_dpiHorizontalFactor = hdpi / baseDpi;
			_dpiVerticalFactor = vdpi / baseDpi;
		}
		Log::info("Display %i: %i:%i x %i:%i (dpi: %f, h: %f, v: %f)", i, dr.x, dr.y, dr.w, dr.h, ddpi, hdpi, vdpi);
	}

	int width = core::Var::get(cfg::ClientWindowWidth, displayMode.w)->intVal();
	int height = core::Var::get(cfg::ClientWindowHeight, displayMode.h)->intVal();

	video::setup();

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
	//SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
#ifdef SDL_HINT_MOUSE_DOUBLE_CLICK_TIME
	SDL_SetHint(SDL_HINT_MOUSE_DOUBLE_CLICK_TIME, "500");
#endif
#ifdef SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS
	SDL_SetHint(SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS, "32");
#endif
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");

	const bool fullscreen = core::Var::getSafe(cfg::ClientFullscreen)->boolVal();

	int flags = SDL_WINDOW_OPENGL;
	if (_showWindow) {
		flags |= SDL_WINDOW_SHOWN;
	} else {
		flags |= SDL_WINDOW_HIDDEN;
	}
	const core::VarPtr& highDPI = core::Var::getSafe(cfg::ClientWindowHghDPI);
	if (highDPI->boolVal()) {
		flags |= SDL_WINDOW_ALLOW_HIGHDPI;
		Log::info("Enable high dpi support");
	} else {
		_dpiFactor = 1.0f;
		_dpiHorizontalFactor = 1.0f;
		_dpiVerticalFactor = 1.0f;
		Log::info("Disable high dpi support");
	}
	if (fullscreen) {
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS;
	}

	const int videoDrivers = SDL_GetNumVideoDrivers();
	for (int i = 0; i < videoDrivers; ++i) {
		Log::info("available driver: %s", SDL_GetVideoDriver(i));
	}

	SDL_Rect displayBounds;
	SDL_GetDisplayBounds(displayIndex, &displayBounds);
	Log::info("driver: %s", SDL_GetCurrentVideoDriver());
	Log::info("found %i displays (use %i at %i:%i)", numDisplays, displayIndex, displayBounds.x, displayBounds.y);
	if (fullscreen && numDisplays > 1) {
		width = displayMode.w;
		height = displayMode.h;
		Log::info("use fake fullscreen for display %i: %i:%i", displayIndex, width, height);
	}

#define InternalCreateWindow() SDL_CreateWindow(_appname.c_str(), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), width, height, flags)

	_window = InternalCreateWindow();
	if (!_window) {
		Log::warn("Failed to get multisampled window - try to disable it");
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		_window = InternalCreateWindow();
		if (!_window) {
			sdlCheckError();
			return core::AppState::InitFailure;
		}
	}

#undef InternalCreateWindow

	_rendererContext = video::createContext(_window);

	SDL_DisableScreenSaver();

	if (SDL_SetWindowBrightness(_window, 1.0f) == -1) {
		sdlCheckError();
	}

	const bool grabMouse = false;
	if (grabMouse && (!fullscreen || numDisplays > 1)) {
		//SDL_SetWindowGrab(_window, SDL_TRUE);
	}

	int screen = 0;
	int modes = SDL_GetNumDisplayModes(screen);
	Log::info("possible display modes:");
	for (int i = 0; i < modes; i++) {
		SDL_GetDisplayMode(screen, i, &displayMode);
		const char *name = SDL_GetPixelFormatName(displayMode.format);
		Log::info("%ix%i@%iHz %s", displayMode.w, displayMode.h, displayMode.refresh_rate, name);
	}

	// some platforms may override or hardcode the resolution - so
	// we have to query it here to get the actual resolution
	int frameBufferWidth, frameBufferHeight;
	SDL_GL_GetDrawableSize(_window, &frameBufferWidth, &frameBufferHeight);
	_aspect = frameBufferWidth / static_cast<float>(frameBufferHeight);
	_frameBufferDimension = glm::ivec2(frameBufferWidth, frameBufferHeight);

	int windowWidth, windowHeight;
	SDL_GetWindowSize(_window, &windowWidth, &windowHeight);
	_windowDimension = glm::ivec2(windowWidth, windowHeight);

	Log::info("resolution (%i:%i) (pixel)", _frameBufferDimension.x, _frameBufferDimension.y);
	Log::info("resolution (%i:%i) (screen)", _windowDimension.x, _windowDimension.y);
	Log::info("dpi factor: %f", _dpiFactor);
	Log::info("dpi factor h: %f", _dpiHorizontalFactor);
	Log::info("dpi factor v: %f", _dpiVerticalFactor);

	const float scaleFactor = _frameBufferDimension.x / (float)_windowDimension.x;
	video::init(_windowDimension.x, _windowDimension.y, scaleFactor);
	video::viewport(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);

	core_trace_gl_init();

	return state;
}

core::AppState WindowedApp::onConstruct() {
	core::AppState state = Super::onConstruct();
	core::Var::get(cfg::ClientMultiSampleBuffers, "0");
	core::Var::get(cfg::ClientMultiSampleSamples, "0");
	core::Var::get(cfg::ClientFullscreen, "true");
	core::Var::get(cfg::ClientWindowHghDPI, "false", core::CV_READONLY);
	core::Var::get(cfg::ClientFog, "true", core::CV_SHADER);
	core::Var::get(cfg::ClientShadowMap, "true", core::CV_SHADER);
	core::Var::get(cfg::ClientDebugShadow, "false", core::CV_SHADER);
	core::Var::get(cfg::ClientShadowMapSize, "512");
	core::Var::get(cfg::ClientDebugShadowMapCascade, "false", core::CV_SHADER);
	core::Var::get(cfg::ClientGamma, "2.2", core::CV_SHADER);
	core::Var::get(cfg::ClientWindowDisplay, 0);
	core::Var::get(cfg::ClientOpenGLVersion, "3.3", core::CV_READONLY);
	core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");
#ifdef DEBUG
	const char *defaultSyncValue = "false";
#else
	const char *defaultSyncValue = "true";
#endif
	core::Var::get(cfg::ClientVSync, defaultSyncValue);

	_keybindingHandler.construct();

	return state;
}

core::AppState WindowedApp::onCleanup() {
	core::Singleton<io::EventHandler>::getInstance().removeObserver(this);
	video::destroyContext(_rendererContext);
	if (_window != nullptr) {
		SDL_DestroyWindow(_window);
	}
	SDL_Quit();
	core_trace_gl_shutdown();

	_keybindingHandler.shutdown();

	return Super::onCleanup();
}

void WindowedApp::showCursor(bool show) {
	SDL_ShowCursor(show ? SDL_TRUE : SDL_FALSE);
}

void WindowedApp::centerMousePosition() {
	SDL_WarpMouseInWindow(_window, frameBufferWidth() / 2, frameBufferHeight() / 2);
}

bool WindowedApp::isRelativeMouseMode() const {
	return SDL_GetRelativeMouseMode();
}

bool WindowedApp::toggleRelativeMouseMode() {
	const bool current = isRelativeMouseMode();
	return setRelativeMouseMode(current ? false : true);
}

bool WindowedApp::setRelativeMouseMode(bool mode) {
	if (mode && !_allowRelativeMouseMode) {
		mode = false;
	}
	SDL_SetRelativeMouseMode(mode ? SDL_TRUE : SDL_FALSE);
	return mode;
}

void WindowedApp::fileDialog(const std::function<void(const std::string&)>& callback, OpenFileMode mode, const std::string& filter) {
	Log::warn("This is not implemented in the base windowed application");
}

WindowedApp* WindowedApp::getInstance() {
	return (WindowedApp*)Super::getInstance();
}

}
