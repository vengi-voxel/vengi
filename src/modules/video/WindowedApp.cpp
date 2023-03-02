/**
 * @file
 */

#include "WindowedApp.h"
#include "Renderer.h"
#include "Shader.h"
#include "ShaderManager.h"
#include "command/Command.h"
#include "core/ArrayLength.h"
#include "core/Assert.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Process.h"
#include "core/Singleton.h"
#include "core/StringUtil.h"
#include "video/Trace.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "gl/GLVersion.h"
#include "io/FormatDescription.h"
#include "io/Filesystem.h"
#include "util/CustomButtonNames.h"
#include "util/KeybindingHandler.h"
#include "util/KeybindingParser.h"
#include "video/EventHandler.h"
#include <glm/common.hpp>
#include <SDL.h>

#ifdef __WINDOWS__
#include <windows.h>
#elif __MACOSX__
extern "C" bool isOSXDarkMode();
#endif

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

WindowedApp::WindowedApp(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize) :
		Super(filesystem, timeProvider, threadPoolSize), _frameBufferDimension(-1), _mouseRelativePos(-1) {
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IOS)
	_singleWindowMode = true;
#else
	_singleWindowMode = false;
#endif
	core::registerBindingContext("ui", core::BindingContext::UI);
	core::registerBindingContext("all", core::BindingContext::All);
}

WindowedApp::~WindowedApp() {
	core::resetBindingContexts();
}

void WindowedApp::onAfterRunning() {
	core_trace_scoped(WindowedAppAfterRunning);
	video::endFrame(_window);
}

bool WindowedApp::handleSDLEvent(SDL_Event& event) {
	switch (event.type) {
	case SDL_QUIT:
		// continue to handle any other following event
		return true;
	case SDL_WINDOWEVENT: {
		SDL_Window *window = SDL_GetWindowFromID(event.window.windowID);
		// we must be the first to handle this - but others should get their chance, too
		if (window == _window && (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
			const int w = event.window.data1;
			const int h = event.window.data2;
			int frameBufferWidth, frameBufferHeight;
			SDL_GL_GetDrawableSize(_window, &frameBufferWidth, &frameBufferHeight);
			_aspect = (float)frameBufferWidth / (float)frameBufferHeight;
			_frameBufferDimension = glm::ivec2(frameBufferWidth, frameBufferHeight);
			_windowDimension = glm::ivec2(w, h);
			const float scaleFactor = (float)_frameBufferDimension.x / (float)_windowDimension.x;
			video::resize(w, h, scaleFactor);
			video::viewport(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
		}
		if (event.window.event == SDL_WINDOWEVENT_MOVED) {
			SDL_RaiseWindow(window);
		}
	}
		// fallthrough
	default: {
		core_trace_scoped(WindowedAppEventHandler);
		const bool running = core::Singleton<io::EventHandler>::getInstance().handleEvent(event);
		if (!running) {
			return true;
		}
		break;
	}
	}
	return false;
}

app::AppState WindowedApp::onRunning() {
	video_trace_scoped(Frame);
	core_trace_scoped(WindowedAppOnRunning);
	if (_keybindingHandler.isPressed(util::button::CUSTOM_SDLK_MOUSE_WHEEL_UP)) {
		handleKeyRelease(util::button::CUSTOM_SDLK_MOUSE_WHEEL_UP, 0);
	}
	if (_keybindingHandler.isPressed(util::button::CUSTOM_SDLK_MOUSE_WHEEL_DOWN)) {
		handleKeyRelease(util::button::CUSTOM_SDLK_MOUSE_WHEEL_DOWN, 0);
	}

	// ignore the state here
	Super::onRunning();

	SDL_GetRelativeMouseState(&_mouseRelativePos.x, &_mouseRelativePos.y);
	SDL_Event event;
	bool quit = false;
	// we are checking the non headless flag here because we assume that a headless windowed
	// application is trying to e.g. render off-screen but without hidden timeouts
	if (_powerSaveMode && _showWindow) {
		bool windowIsHidden = SDL_GetWindowFlags(_window) & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED);
		while (windowIsHidden) {
			if (SDL_WaitEvent(&event) == 1) {
				quit = handleSDLEvent(event);
				windowIsHidden = SDL_GetWindowFlags(_window) & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED);
			}
		}
	}
	while (SDL_PollEvent(&event)) {
		quit |= handleSDLEvent(event);
	}

	if (quit) {
		Log::debug("Quitting the application");
		requestQuit();
		return app::AppState::Running;
	}

	core_trace_scoped(WindowedAppStartFrame);
	video::startFrame(_window, _rendererContext);
	core::Singleton<ShaderManager>::getInstance().update();

	const uint64_t end = core::TimeProvider::highResTime();
	const double frameDelta = (double)(end - _timeProvider->highResTickTime()) / (double)core::TimeProvider::highResTimeResolution() * 1000.0;
	_fps = 1.0 / frameDelta;

	video_trace_frame_end();

	return app::AppState::Running;
}

void WindowedApp::onWindowClose(void *windowHandle) {
	// if the main window is going to get closed, quit the application
	if (_window == windowHandle) {
		requestQuit();
	}
}

// https://stackoverflow.com/questions/25207077/how-to-detect-if-os-x-is-in-dark-mode
// https://wiki.archlinux.org/title/Dark_mode_switching#gsettings
bool WindowedApp::isDarkMode() const {
#ifdef __MACOSX__
	return isOSXDarkMode();
#elif __LINUX__
	core::Process process;
	core::DynamicArray<core::String> arguments;
	arguments.push_back("get");
	arguments.push_back("org.gnome.desktop.interface");
	arguments.push_back("gtk-theme");
	char outputBuf[4096] = "";
	const int exitCode = process.exec("/usr/bin/gsettings", arguments, nullptr, outputBuf, sizeof(outputBuf));
	if (exitCode == 0) {
		return core::string::icontains(outputBuf, "dark");
	}
	return true;
#elif __WINDOWS__
	HKEY hkey;
	if (RegOpenKey(HKEY_CURRENT_USER, R"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)", &hkey) == ERROR_SUCCESS) {
		DWORD value = 0;
		DWORD size = sizeof(DWORD);
		auto error = RegQueryValueEx(hkey, "AppsUseLightTheme", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size);
		if (error == ERROR_SUCCESS) {
			return value != 0;
		}
	}
	return true;
#else
	return true;
#endif
}

bool WindowedApp::onKeyRelease(int32_t key, int16_t modifier) {
	return handleKeyRelease(key, modifier);
}

bool WindowedApp::handleKeyRelease(int32_t key, int16_t /*modifier*/) {
	// don't use modifier here - this is the modifier that was released. But
	// we need the currently pressed modifier mask
	return _keybindingHandler.execute(key, SDL_GetModState(), false, nowSeconds());
}

bool WindowedApp::handleKeyPress(int32_t key, int16_t modifier, uint16_t count) {
	return _keybindingHandler.execute(key, modifier, true, nowSeconds(), count);
}

bool WindowedApp::onMouseWheel(int32_t x, int32_t y) {
	const int32_t key = y < 0 ? util::button::CUSTOM_SDLK_MOUSE_WHEEL_UP : util::button::CUSTOM_SDLK_MOUSE_WHEEL_DOWN;
	const int16_t mod = SDL_GetModState();
	if (handleKeyPress(key, mod)) {
		return true;
	}
	return false;
}

void WindowedApp::onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) {
	handleKeyPress(CUSTOM_SDL_KEYCODE(button), SDL_GetModState(), clicks);
}

void WindowedApp::onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) {
	handleKeyRelease(CUSTOM_SDL_KEYCODE(button), SDL_GetModState());
}

bool WindowedApp::onKeyPress(int32_t key, int16_t modifier) {
	return handleKeyPress(key, modifier);
}

core::String WindowedApp::getKeyBindingsString(const char *cmd) const {
	return _keybindingHandler.getKeyBindingsString(cmd);
}

SDL_Window *WindowedApp::createWindow(int width, int height, int displayIndex, uint32_t flags) {
	return SDL_CreateWindow(_appname.c_str(), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), width, height, flags);
}

app::AppState WindowedApp::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		sdlCheckError();
		return app::AppState::InitFailure;
	}

	if (!_keybindingHandler.init()) {
		Log::error("Failed to initialize the key binding handler");
		return app::AppState::InitFailure;
	}
	if (!_keybindingHandler.load(_keybindingsVersion)) {
		Log::debug("Failed to load the keybindings");
	}

	core::Singleton<io::EventHandler>::getInstance().registerObserver(this);

	Log::debug("CPU count: %d", SDL_GetCPUCount());
	Log::debug("CacheLine size: %d", SDL_GetCPUCacheLineSize());
	Log::debug("RDTSC: %d", SDL_HasRDTSC());
	Log::debug("Altivec: %d", SDL_HasAltiVec());
	Log::debug("MMX: %d", SDL_HasMMX());
	Log::debug("3DNow: %d", SDL_Has3DNow());
	Log::debug("SSE: %d", SDL_HasSSE());
	Log::debug("SSE2: %d", SDL_HasSSE2());
	Log::debug("SSE3: %d", SDL_HasSSE3());
	Log::debug("SSE4.1: %d", SDL_HasSSE41());
	Log::debug("SSE4.2: %d", SDL_HasSSE42());
	Log::debug("AVX: %d", SDL_HasAVX());
	Log::debug("AVX2: %d", SDL_HasAVX2());
#if SDL_VERSION_ATLEAST(2, 0, 9)
	Log::debug("NEON: %d", SDL_HasNEON());
#endif
	Log::debug("RAM: %d MB", SDL_GetSystemRAM());

	SDL_DisplayMode displayMode;
	const int numDisplays = core_max(0, SDL_GetNumVideoDisplays());
	const int displayIndex = glm::clamp(core::Var::getSafe(cfg::ClientWindowDisplay)->intVal(), 0, core_max(0, numDisplays - 1));
	Log::debug("Try to use display %i", displayIndex);
	if (SDL_GetDesktopDisplayMode(displayIndex, &displayMode) == -1) {
		Log::error("%s", SDL_GetError());
		return app::AppState::InitFailure;
	}

	for (int i = 0; i < numDisplays; ++i) {
		SDL_Rect dr;
		if (SDL_GetDisplayBounds(i, &dr) == -1) {
			continue;
		}
		float ddpi = -1.0f;
		float hdpi = -1.0f;
		float vdpi = -1.0f;
		SDL_GetDisplayDPI(i, &ddpi, &hdpi, &vdpi);
		Log::debug("Display %i: %i:%i x %i:%i (dpi: %f, h: %f, v: %f)", i, dr.x, dr.y, dr.w, dr.h, ddpi, hdpi, vdpi);
	}

	video::setup();

	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
#ifdef SDL_HINT_MOUSE_DOUBLE_CLICK_TIME
	SDL_SetHint(SDL_HINT_MOUSE_DOUBLE_CLICK_TIME, "500");
#endif
#ifdef SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS
	SDL_SetHint(SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS, "32");
#endif
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");

	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
	if (!_showWindow) {
		flags |= SDL_WINDOW_HIDDEN;
	}
	SDL_Rect displayBounds;
	if (SDL_GetDisplayUsableBounds(displayIndex, &displayBounds) < 0) {
		Log::error("Failed to query usable display bounds: %s", SDL_GetError());
		displayBounds.h = displayBounds.w = displayBounds.x = displayBounds.y = 0;
	}
	const core::VarPtr& highDPI = core::Var::getSafe(cfg::ClientWindowHighDPI);
	if (highDPI->boolVal()) {
		flags |= SDL_WINDOW_ALLOW_HIGHDPI;
		Log::debug("Enable high dpi support");
	} else {
		Log::debug("Disable high dpi support");
	}

	const int videoDrivers = SDL_GetNumVideoDrivers();
	for (int i = 0; i < videoDrivers; ++i) {
		Log::debug("available driver: %s", SDL_GetVideoDriver(i));
	}

	Log::debug("driver: %s", SDL_GetCurrentVideoDriver());
	Log::debug("found %i displays (use %i at %i:%i)", numDisplays, displayIndex, displayBounds.x, displayBounds.y);

	const int width = core_max(1024, displayBounds.w);
	const int height = core_max(768, displayBounds.h);
	_window = createWindow(width, height, displayIndex, flags);
	if (!_window) {
		Log::warn("Failed to get multisampled window - try to disable it");
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		_window = createWindow(width, height, displayIndex, flags);
		if (!_window) {
			sdlCheckError();
			return app::AppState::InitFailure;
		}
	}

	SDL_MaximizeWindow(_window);

	if (displayIndex != SDL_GetWindowDisplayIndex(_window)) {
		Log::error("Failed to create window at display %i", displayIndex);
	}

	_rendererContext = video::createContext(_window);
	if (_rendererContext == nullptr) {
		sdlCheckError();
		return app::AppState::InitFailure;
	}

	int screen = 0;
	int modes = SDL_GetNumDisplayModes(screen);
	Log::debug("possible display modes:");
	for (int i = 0; i < modes; i++) {
		SDL_GetDisplayMode(screen, i, &displayMode);
		const char *name = SDL_GetPixelFormatName(displayMode.format);
		Log::debug("%ix%i@%iHz %s", displayMode.w, displayMode.h, displayMode.refresh_rate, name);
	}

	// some platforms may override or hardcode the resolution - so
	// we have to query it here to get the actual resolution
	int frameBufferWidth, frameBufferHeight;
	SDL_GL_GetDrawableSize(_window, &frameBufferWidth, &frameBufferHeight);
	_aspect = (float)frameBufferWidth / (float)frameBufferHeight;
	_frameBufferDimension = glm::ivec2(frameBufferWidth, frameBufferHeight);

	int windowWidth, windowHeight;
	SDL_GetWindowSize(_window, &windowWidth, &windowHeight);
	_windowDimension = glm::ivec2(windowWidth, windowHeight);

	Log::debug("resolution (%i:%i) (pixel)", _frameBufferDimension.x, _frameBufferDimension.y);
	Log::debug("resolution (%i:%i) (screen)", _windowDimension.x, _windowDimension.y);

	const float scaleFactor = (float)_frameBufferDimension.x / (float)_windowDimension.x;
	video::init(_windowDimension.x, _windowDimension.y, scaleFactor);
	video::viewport(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);

	video_trace_init();

	return state;
}

app::AppState WindowedApp::onConstruct() {
	app::AppState state = Super::onConstruct();
	core::Var::get(cfg::ClientMultiSampleBuffers, "0");
	core::Var::get(cfg::ClientMultiSampleSamples, "0");
	core::Var::get(cfg::ClientWindowHighDPI, "false", core::CV_READONLY);
	core::Var::get(cfg::ClientShadowMap, "true", core::CV_SHADER, "Activate shadow map", core::Var::boolValidator);
	core::Var::get(cfg::ClientBloom, "true", "Activate bloom post processing", core::Var::boolValidator);
	core::Var::get(cfg::ClientDebugShadow, "false", core::CV_SHADER, "Activate shadow debug rendering", core::Var::boolValidator);
	core::Var::get(cfg::ClientShadowMapSize, "1024");
	core::Var::get(cfg::ClientDebugShadowMapCascade, "false", core::CV_SHADER, "Activate cascade shadow map debug rendering", core::Var::boolValidator);
	core::Var::get(cfg::ClientGamma, "1.0", core::CV_SHADER, "Gamma correction");
	core::Var::get(cfg::ClientWindowDisplay, 0);
	core::Var::get(cfg::ClientOpenGLVersion, "3.3", core::CV_READONLY);
	core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");
	core::Var::get(cfg::RenderOutline, "false", core::CV_SHADER, "Render voxel outline", core::Var::boolValidator);
	core::Var::get(cfg::ClientVSync, "true", "Limit the framerate to the monitor refresh rate", core::Var::boolValidator);
	core::Var::get(cfg::ClientDebugSeverity, "0", 0u, "0 disables it, 1 only highest severity, 2 medium severity, 3 everything");
	core::Var::get(cfg::ClientCameraZoomSpeed, "0.1");

	core::Var::get(cfg::ClientCameraMaxZoom, "1000.0", 0u, "", [](const core::String &val) {
		const float fv = core::string::toFloat(val);
		return fv > 1.0f && fv <= 1000.0f;
	});
	core::Var::get(cfg::ClientCameraMinZoom, "0.1", 0u, "", [](const core::String &val) {
		const float fv = core::string::toFloat(val);
		return fv > 0.0001f && fv < 1000.0f;
	});

	command::Command::registerCommand("minimize", [&] (const command::CmdArgs& args) {
		minimize();
	});

	video::construct();

	_keybindingHandler.construct();

	return state;
}

app::AppState WindowedApp::onCleanup() {
	core::Singleton<io::EventHandler>::getInstance().removeObserver(this);
	video::destroyContext(_rendererContext);
	if (_window != nullptr) {
		SDL_DestroyWindow(_window);
	}
	SDL_Quit();
	video_trace_shutdown();

	_keybindingHandler.shutdown(_keybindingsVersion);

	return Super::onCleanup();
}

void WindowedApp::resetKeybindings() {
	_keybindingHandler.reset(_keybindingsVersion);
	_keybindingHandler.load(_keybindingsVersion);
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

void WindowedApp::fileDialog(const FileDialogSelectionCallback& callback, const FileDialogOptions& options, OpenFileMode mode, const io::FormatDescription* formats, const core::String &filename) {
	Log::warn("This is not implemented in the base windowed application");
}

void WindowedApp::minimize() {
	SDL_MinimizeWindow(_window);
}

WindowedApp* WindowedApp::getInstance() {
	return (WindowedApp*)Super::getInstance();
}

}
