/**
 * @file
 */

#include "WindowedApp.h"
#include "Renderer.h"
#include "ShaderManager.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Singleton.h"
#include "core/StringUtil.h"
#if !SDL_VERSION_ATLEAST(3, 2, 0)
#include "core/Process.h"
#include "io/BufferedReadWriteStream.h"
#endif
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "util/CustomButtonNames.h"
#include "util/KeybindingHandler.h"
#include "video/EventHandler.h"
#include "video/Trace.h"
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_mouse.h>
#include <glm/common.hpp>
#ifdef USE_VK_RENDERER
#include <SDL_vulkan.h>
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#elif __APPLE__
extern "C" bool isOSXDarkMode();
#endif

#if SDL_VERSION_ATLEAST(3, 2, 0)
#define SDL_GL_GetDrawableSize SDL_GetWindowSizeInPixels
#define SDL_GetWindowDisplayIndex SDL_GetDisplayForWindow
#define SDL_QUIT SDL_EVENT_QUIT
#define SDL_WINDOW_ALLOW_HIGHDPI SDL_WINDOW_HIGH_PIXEL_DENSITY
#define sdlCheckError() checkSDLError(__FILE__, __LINE__, "UNKNOWN")
#else
#include <SDL.h>
#define sdlCheckError() checkSDLError(__FILE__, __LINE__, SDL_FUNCTION)
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
} // namespace

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
	video_trace_frame_end();

	const double frameStartSeconds = _timeProvider->tickSeconds();
	const double frameCurrentSeconds = _timeProvider->nowSeconds();
	const double frameDeltaSeconds = frameCurrentSeconds - frameStartSeconds;
	_fps = 1.0 / frameDeltaSeconds;
}

bool WindowedApp::handleSDLEvent(SDL_Event &event) {
	switch (event.type) {
	case SDL_QUIT:
		// continue to handle any other following event
		return true;
#if SDL_VERSION_ATLEAST(3, 2, 0)
	case SDL_EVENT_WINDOW_RESIZED:
		// fallthrough
	case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
		SDL_Window *window = SDL_GetWindowFromID(event.window.windowID);
		if (window == _window) {
			const int w = event.window.data1;
			const int h = event.window.data2;
			int frameBufferWidth, frameBufferHeight;
			SDL_GetWindowSizeInPixels(_window, &frameBufferWidth, &frameBufferHeight);
			_aspect = (float)frameBufferWidth / (float)frameBufferHeight;
			_frameBufferDimension = glm::ivec2(frameBufferWidth, frameBufferHeight);
			_windowDimension = glm::ivec2(w, h);
			const float scaleFactor = (float)_frameBufferDimension.x / (float)_windowDimension.x;
			video::resize(w, h, scaleFactor);
			video::viewport(0, 0, _frameBufferDimension.x, _frameBufferDimension.y);
		}
		break;
	}
	case SDL_EVENT_WINDOW_MOVED: {
		SDL_Window *window = SDL_GetWindowFromID(event.window.windowID);
		SDL_RaiseWindow(window);
		break;
	}
#else
	case SDL_WINDOWEVENT: {
		SDL_Window *window = SDL_GetWindowFromID(event.window.windowID);
		// we must be the first to handle this - but others should get their chance, too
		if (window == _window && (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)) {
			const int w = event.window.data1;
			const int h = event.window.data2;
			int frameBufferWidth, frameBufferHeight;
#ifdef USE_GL_RENDERER
			SDL_GL_GetDrawableSize(_window, &frameBufferWidth, &frameBufferHeight);
#elif defined(USE_VK_RENDERER)
			SDL_Vulkan_GetDrawableSize(_window, &frameBufferWidth, &frameBufferHeight);
#else
#error "renderer not supported"
#endif
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
#endif
	default: {
		core_trace_scoped(WindowedAppEventHandler);
		const bool running = core::Singleton<video::EventHandler>::getInstance().handleEvent(event);
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
	if (isPressed(util::button::CUSTOM_SDLK_MOUSE_WHEEL_UP)) {
		handleKeyRelease(util::button::CUSTOM_SDLK_MOUSE_WHEEL_UP, 0);
	}
	if (isPressed(util::button::CUSTOM_SDLK_MOUSE_WHEEL_DOWN)) {
		handleKeyRelease(util::button::CUSTOM_SDLK_MOUSE_WHEEL_DOWN, 0);
	}
	if (isPressed(util::button::CUSTOM_SDLK_MOUSE_WHEEL_LEFT)) {
		handleKeyRelease(util::button::CUSTOM_SDLK_MOUSE_WHEEL_LEFT, 0);
	}
	if (isPressed(util::button::CUSTOM_SDLK_MOUSE_WHEEL_RIGHT)) {
		handleKeyRelease(util::button::CUSTOM_SDLK_MOUSE_WHEEL_RIGHT, 0);
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
#if SDL_VERSION_ATLEAST(3, 2, 0)
	const SDL_SystemTheme theme = SDL_GetSystemTheme();
	return theme == SDL_SYSTEM_THEME_DARK || theme == SDL_SYSTEM_THEME_UNKNOWN;
#else
#ifdef __APPLE__
	return isOSXDarkMode();
#elif __linux__
	core::DynamicArray<core::String> arguments;
	arguments.push_back("get");
	arguments.push_back("org.gnome.desktop.interface");
	arguments.push_back("gtk-theme");
	io::BufferedReadWriteStream stream(4096);
	const int exitCode = core::Process::exec("/usr/bin/gsettings", arguments, nullptr, &stream);
	if (exitCode == 0) {
		stream.seek(0);
		core::String output;
		stream.readString(stream.size(), output);
		Log::debug("gsettings gtk-theme: '%s'", output.c_str());
		return core::string::icontains(output, "dark");
	} else {
		Log::warn("Failed to execute gsettings: %i", exitCode);
	}
	return true;
#elif defined(_WIN32) || defined(__CYGWIN__)
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
#endif
}

bool WindowedApp::onKeyRelease(void *windowHandle, int32_t key, int16_t modifier) {
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

bool WindowedApp::onMouseWheel(void *windowHandle, float x, float y, int32_t mouseId) {
	int32_t key;
	if (y < 0.0f)
		key = util::button::CUSTOM_SDLK_MOUSE_WHEEL_UP;
	else if (y > 0.0f)
		key = util::button::CUSTOM_SDLK_MOUSE_WHEEL_DOWN;
	else if (x < 0.0f)
		key = util::button::CUSTOM_SDLK_MOUSE_WHEEL_LEFT;
	else if (x > 0.0f)
		key = util::button::CUSTOM_SDLK_MOUSE_WHEEL_RIGHT;
	else
		return false;
	const int16_t mod = SDL_GetModState();
	if (handleKeyPress(key, mod)) {
		return true;
	}
	return false;
}

void WindowedApp::onMouseButtonPress(void *windowHandle, int32_t x, int32_t y, uint8_t button, uint8_t clicks, int32_t mouseId) {
	handleKeyPress(CUSTOM_SDL_KEYCODE(button), SDL_GetModState(), clicks);
}

void WindowedApp::onMouseButtonRelease(void *windowHandle, int32_t x, int32_t y, uint8_t button, int32_t mouseId) {
	handleKeyRelease(CUSTOM_SDL_KEYCODE(button), SDL_GetModState());
}

bool WindowedApp::onFingerPress(void *windowHandle, int64_t finger, float x, float y, float pressure, uint32_t timestamp) {
	return false;
}

bool WindowedApp::onFingerRelease(void *windowHandle, int64_t finger, float x, float y, uint32_t timestamp) {
	return false;
}

void WindowedApp::onFingerMotion(void *windowHandle, int64_t finger, float x, float y, float dx, float dy, float pressure, uint32_t timestamp) {
}

void WindowedApp::onPenAxis(void *windowHandle, uint32_t pen, float x, float y, PenAxis axis, float value) {
}

void WindowedApp::onPenDown(void *windowHandle, uint32_t pen, float x, float y, bool eraser) {
	handleKeyPress(eraser ? util::button::CUSTOM_SDLK_PEN_ERASER : util::button::CUSTOM_SDLK_PEN_TIP, SDL_GetModState());
}

void WindowedApp::onPenUp(void *windowHandle, uint32_t pen, float x, float y, bool eraser) {
	handleKeyRelease(eraser ? util::button::CUSTOM_SDLK_PEN_ERASER : util::button::CUSTOM_SDLK_PEN_TIP, SDL_GetModState());
}

void WindowedApp::onPenButtonDown(void *windowHandle, uint32_t pen, float x, float y, uint8_t button) {
	if (button >= 4) {
		return;
	}
	handleKeyPress(util::button::CUSTOM_SDLK_PEN_BUTTON0 + button, SDL_GetModState());
}

void WindowedApp::onPenButtonUp(void *windowHandle, uint32_t pen, float x, float y, uint8_t button) {
	if (button >= 4) {
		return;
	}
	handleKeyRelease(util::button::CUSTOM_SDLK_PEN_BUTTON0 + button, SDL_GetModState());
}

void WindowedApp::onPenProximityIn(void *windowHandle, uint32_t pen) {
}

void WindowedApp::onPenProximityOut(void *windowHandle, uint32_t pen) {
}

void WindowedApp::onPenMotion(void *windowHandle, uint32_t pen, float x, float y) {
}

bool WindowedApp::onKeyPress(void *windowHandle, int32_t key, int16_t modifier) {
	return handleKeyPress(key, modifier);
}

core::String WindowedApp::getKeyBindingsString(const char *cmd) const {
	return _keybindingHandler.getKeyBindingsString(cmd);
}

SDL_Window *WindowedApp::createWindow(int width, int height, int displayIndex, uint32_t flags) {
	const core::String windowName = fullAppname();
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, windowName.c_str());
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
	SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, flags);
	SDL_Window *window = SDL_CreateWindowWithProperties(props);
	SDL_DestroyProperties(props);
	return window;
#else
	return SDL_CreateWindow(windowName.c_str(), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), width, height, flags);
#endif
}

app::AppState WindowedApp::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

#if SDL_VERSION_ATLEAST(3, 2, 0)
	if (!SDL_Init(SDL_INIT_VIDEO)) {
#else
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
#endif
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

	core::Singleton<video::EventHandler>::getInstance().registerObserver(this);

#if SDL_VERSION_ATLEAST(3, 2, 0)
	const int displayIndex = 0;
#else
	Log::debug("CPU count: %d", SDL_GetCPUCount());
	Log::debug("CacheLine size: %d", SDL_GetCPUCacheLineSize());
	Log::debug("Altivec: %d", SDL_HasAltiVec());
	Log::debug("MMX: %d", SDL_HasMMX());
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
	const int numDisplays = core_max(0, SDL_GetNumVideoDisplays());
	const int displayIndex = glm::clamp(core::Var::getSafe(cfg::ClientWindowDisplay)->intVal(), 0, core_max(0, numDisplays - 1));
	Log::debug("Try to use display %i", displayIndex);
	Log::debug("found %i displays (use %i)", numDisplays, displayIndex);
#endif

	video::setup();

#ifdef SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
#endif
#ifdef SDL_HINT_MOUSE_DOUBLE_CLICK_TIME
	SDL_SetHint(SDL_HINT_MOUSE_DOUBLE_CLICK_TIME, "500");
#endif
#ifdef SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS
	SDL_SetHint(SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS, "32");
#endif
#ifdef SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
#endif
#ifdef SDL_HINT_VIDEO_ALLOW_SCREENSAVER
	SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
#endif
	// done in application.manifest.in
	// #ifdef SDL_HINT_WINDOWS_DPI_AWARENESS
	// 	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
	// #endif

	int flags = SDL_WINDOW_RESIZABLE;
#ifdef USE_GL_RENDERER
	flags |= SDL_WINDOW_OPENGL;
#elif USE_VK_RENDERER
	flags |= SDL_WINDOW_VULKAN;
#else
#error "renderer not supported"
#endif
	if (!_showWindow) {
		flags |= SDL_WINDOW_HIDDEN;
	}
	SDL_Rect displayBounds;
	displayBounds.h = displayBounds.w = displayBounds.x = displayBounds.y = 0;
	if (_fullScreenApplication) {
#if SDL_VERSION_ATLEAST(3, 2, 0)
		SDL_DisplayID primaryDisplay = SDL_GetPrimaryDisplay();
		if (!SDL_GetDisplayUsableBounds(primaryDisplay, &displayBounds)) {
			Log::error("Failed to query usable display bounds at %i: %s", primaryDisplay, SDL_GetError());
#else
		if (SDL_GetDisplayUsableBounds(displayIndex, &displayBounds) < 0) {
			Log::error("Failed to query usable display bounds at %i: %s", displayIndex, SDL_GetError());
#endif
			displayBounds.h = displayBounds.w = displayBounds.x = displayBounds.y = 0;
		}
	}
	const core::VarPtr &highDPI = core::Var::getSafe(cfg::ClientWindowHighDPI);
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

	const int width = core_max(_windowWidth, displayBounds.w);
	const int height = core_max(_windowHeight, displayBounds.h);
	_window = createWindow(width, height, displayIndex, flags);
	if (!_window) {
		Log::warn("Failed to get multisampled window - retrying without multisampling");
#ifdef USE_GL_RENDERER
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
#elif USE_VK_RENDERER
		// TODO: VULKAN
#else
#error "renderer not supported"
#endif
		_window = createWindow(width, height, displayIndex, flags);
		if (!_window) {
			sdlCheckError();
			return app::AppState::InitFailure;
		}
	}

	if (_fullScreenApplication) {
		SDL_MaximizeWindow(_window);
	}

#if !SDL_VERSION_ATLEAST(3, 2, 0)
	int actualDisplayIndex = (int)SDL_GetWindowDisplayIndex(_window);
	if (displayIndex != actualDisplayIndex) {
		Log::error("Failed to create window at display %i (got %i)", displayIndex, actualDisplayIndex);
	}
#endif

	_rendererContext = video::createContext(_window);
	if (_rendererContext == nullptr) {
		sdlCheckError();
		return app::AppState::InitFailure;
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
	_keyboardLayout = video::detectKeyboardLayout();

	video_trace_init();

	return state;
}

app::AppState WindowedApp::onConstruct() {
	app::AppState state = Super::onConstruct();
	core::Var::get(cfg::ClientMultiSampleBuffers, "0");
	core::Var::get(cfg::ClientMultiSampleSamples, "0");
	core::Var::get(cfg::ClientWindowHighDPI, "true", core::CV_READONLY);
	core::Var::get(cfg::ClientShadowMap, "true", core::CV_SHADER, _("Activate shadow map"), core::Var::boolValidator);
	core::Var::get(cfg::RenderCheckerBoard, "false", core::CV_SHADER, _("Activate checkerboard rendering"), core::Var::boolValidator);
	core::Var::get(cfg::RenderCullBuffers, "false", _("Activate culling for buffer parts"), core::Var::boolValidator);
	core::Var::get(cfg::RenderCullNodes, "true", _("Activate culling for scene nodes"), core::Var::boolValidator);
	core::Var::get(cfg::ClientBloom, "true", _("Activate bloom post processing"), core::Var::boolValidator);
	core::Var::get(cfg::ClientDebugShadow, "false", core::CV_SHADER, _("Activate shadow debug rendering"), core::Var::boolValidator);
	core::Var::get(cfg::ClientShadowMapSize, "4096");
	core::Var::get(cfg::ClientDebugShadowMapCascade, "false", core::CV_SHADER, _("Activate cascade shadow map debug rendering"), core::Var::boolValidator);
	core::Var::get(cfg::ClientGamma, "1.0", core::CV_SHADER, _("Gamma correction"));
	core::Var::get(cfg::ClientWindowDisplay, 0);
	core::Var::get(cfg::ClientOpenGLVersion, "3.3", core::CV_READONLY);
	core::Var::get(cfg::RenderOutline, "false", core::CV_SHADER, _("Render voxel outline"), core::Var::boolValidator);
	core::Var::get(cfg::RenderNormals, "false", core::CV_SHADER, _("Render voxel normals"), core::Var::boolValidator);
	core::Var::get(cfg::ToneMapping, "0", core::CV_SHADER, _("Activate tone mapping"), core::Var::minMaxValidator<0, 3>);
	core::Var::get(cfg::ClientVSync, "true", _("Limit the framerate to the monitor refresh rate"), core::Var::boolValidator);
	core::Var::get(cfg::ClientDebugSeverity, "0", 0u, _("0 disables it, 1 only highest severity, 2 medium severity, 3 everything"));
	core::Var::get(cfg::ClientMouseRotationSpeed, "0.01");
	core::Var::get(cfg::ClientCameraZoomSpeed, "0.1");

	// Default anisotropy used for framebuffer-created textures. -1 means use device max.
	core::Var::get(cfg::MaxAnisotropy, "-1", 0u, "", [](const core::String &val) {
		const float fv = core::string::toFloat(val);
		// allow -1 (use device max) or any value >= 0
		return fv == -1.0f || fv >= 0.0f;
	});

	core::Var::get(cfg::ClientCameraMaxZoom, "1000.0", 0u, "", [](const core::String &val) {
		const float fv = core::string::toFloat(val);
		return fv > 1.0f && fv <= 1000.0f;
	});
	core::Var::get(cfg::ClientCameraMinZoom, "0.001", 0u, "", [](const core::String &val) {
		const float fv = core::string::toFloat(val);
		return fv > 0.0001f && fv < 1000.0f;
	});

	command::Command::registerCommand("minimize")
		.setHandler([&] (const command::CommandArgs& args) {
			minimize();
		}).setHelp(_("Minimize the window"));

	video::construct();

	_keybindingHandler.construct();

	return state;
}

app::AppState WindowedApp::onCleanup() {
	core::Singleton<video::EventHandler>::getInstance().removeObserver(this);
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
}

void WindowedApp::openKeybindings() {
	_keybindingHandler.openKeybindings(_keybindingsVersion);
}

void WindowedApp::showCursor(bool show) {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	if (show) {
		SDL_ShowCursor();
	} else {
		SDL_HideCursor();
	}
#else
	SDL_ShowCursor(show ? SDL_TRUE : SDL_FALSE);
#endif
}

void WindowedApp::centerMousePosition() {
	SDL_WarpMouseInWindow(_window, frameBufferWidth() / 2, frameBufferHeight() / 2);
}

bool WindowedApp::isRelativeMouseMode() const {
#if SDL_VERSION_ATLEAST(3, 2, 0)
	return SDL_GetWindowRelativeMouseMode(_window);
#else
	return SDL_GetRelativeMouseMode();
#endif
}

bool WindowedApp::toggleRelativeMouseMode() {
	const bool current = isRelativeMouseMode();
	return setRelativeMouseMode(current ? false : true);
}

bool WindowedApp::setRelativeMouseMode(bool mode) {
	if (mode && !_allowRelativeMouseMode) {
		mode = false;
	}
#if SDL_VERSION_ATLEAST(3, 2, 0)
	SDL_SetWindowRelativeMouseMode(_window, mode);
#else
	SDL_SetRelativeMouseMode(mode ? SDL_TRUE : SDL_FALSE);
#endif
	return mode;
}

void WindowedApp::fileDialog(const FileDialogSelectionCallback& callback, const FileDialogOptions& options, OpenFileMode mode, const io::FormatDescription* formats, const core::String &filename) {
	Log::warn("This is not implemented in the base windowed application");
}

void WindowedApp::minimize() {
	SDL_MinimizeWindow(_window);
}

WindowedApp *WindowedApp::getInstance() {
	return (WindowedApp *)Super::getInstance();
}

} // namespace video
