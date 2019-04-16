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
		Super(metric, filesystem, eventBus, timeProvider), _dimension(-1), _mousePos(-1), _mouseRelativePos(-1) {
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

void WindowedApp::onWindowResize() {
	int _width, _height;
	SDL_GetWindowSize(_window, &_width, &_height);
	_aspect = _width / static_cast<float>(_height);
	_dimension = glm::ivec2(_width, _height);
	video::viewport(0, 0, _width, _height);
}

bool WindowedApp::onKeyRelease(int32_t key, int16_t modifier) {
	bool handled = false;
	int16_t code = 0;
	switch (key) {
	case SDLK_LCTRL:
		code = KMOD_LCTRL;
		break;
	case SDLK_RCTRL:
		code = KMOD_RCTRL;
		break;
	case SDLK_LSHIFT:
		code = KMOD_LSHIFT;
		break;
	case SDLK_RSHIFT:
		code = KMOD_RSHIFT;
		break;
	case SDLK_LALT:
		code = KMOD_LALT;
		break;
	case SDLK_RALT:
		code = KMOD_RALT;
		break;
	}
	if (code != 0) {
		for (auto& b : _bindings) {
			const util::CommandModifierPair& pair = b.second;
			const int32_t commandKey = b.first;
			if (pair.command[0] != '+') {
				// no action button command
				continue;
			}
			if (!util::isValidForBinding(code, pair.command, pair.modifier)) {
				continue;
			}
			if (!isPressed(commandKey)) {
				continue;
			}
			core::Command::execute("-%s %i %" PRId64, &(pair.command.c_str()[1]), commandKey, _now);
			// first try to execute commands with all the modifiers that are currently pressed
			if (!util::executeCommandsForBinding(_bindings, commandKey, SDL_GetModState(), _now)) {
				// if no binding was found, execute without modifier
				util::executeCommandsForBinding(_bindings, commandKey, 0, _now);
			}
		}
	}
	auto range = _bindings.equal_range(key);
	for (auto i = range.first; i != range.second; ++i) {
		const std::string& command = i->second.command;
		if (command[0] == '+') {
			core::Command::execute("-%s %i %" PRId64, &(command.c_str()[1]), key, _now);
			handled = true;
		}
	}
	_keys.erase(key);
	return handled;
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

	_keys.insert(key);

	int16_t code = 0;
	switch (key) {
	case SDLK_LCTRL:
		code = KMOD_LCTRL;
		break;
	case SDLK_RCTRL:
		code = KMOD_RCTRL;
		break;
	case SDLK_LSHIFT:
		code = KMOD_LSHIFT;
		break;
	case SDLK_RSHIFT:
		code = KMOD_RSHIFT;
		break;
	case SDLK_LALT:
		code = KMOD_LALT;
		break;
	case SDLK_RALT:
		code = KMOD_RALT;
		break;
	}
	if (code != 0) {
		std::unordered_set<int32_t> recheck;
		for (auto& b : _bindings) {
			const util::CommandModifierPair& pair = b.second;
			const int32_t commandKey = b.first;
			if (pair.command[0] != '+') {
				// no action button command
				continue;
			}
			if (pair.modifier == 0) {
				continue;
			}
			if (!isPressed(commandKey)) {
				continue;
			}
			if (!util::isValidForBinding(modifier, pair.command, pair.modifier)) {
				continue;
			}
			core::Command::execute("%s %i %" PRId64, pair.command.c_str(), commandKey, _now);
			recheck.insert(commandKey);
		}
		for (int32_t checkKey : recheck) {
			auto range = _bindings.equal_range(checkKey);
			for (auto i = range.first; i != range.second; ++i) {
				const util::CommandModifierPair& pair = i->second;
				if (pair.modifier != 0) {
					continue;
				}
				core::Command::execute("-%s %i %" PRId64, &(pair.command.c_str()[1]), checkKey, _now);
			}
		}
	}

	if (!util::executeCommandsForBinding(_bindings, key, modifier, _now)) {
		return util::executeCommandsForBinding(_bindings, key, 0, _now);
	}
	return true;
}

bool WindowedApp::resolveKeyBindings(const char *cmd, int16_t* modifier, int32_t* key) const {
	for (const auto& b : _bindings) {
		const util::CommandModifierPair& pair = b.second;
		if (strcmp(pair.command.c_str(), cmd)) {
			continue;
		}

		if (modifier != nullptr) {
			*modifier = pair.modifier;
		}
		if (key != nullptr) {
			*key = b.first;
		}
		return true;
	}
	return false;
}

const struct ModifierMapping {
	int16_t modifier;
	const char *name;
} MODIFIERMAPPING[] = {
	{KMOD_LSHIFT, "LSHIFT"},
	{KMOD_RSHIFT, "RSHIFT"},
	{KMOD_LCTRL, "LCTRL"},
	{KMOD_RCTRL, "RCTRL"},
	{KMOD_LALT, "LALT"},
	{KMOD_RALT, "RALT"},
	{KMOD_ALT, "ALT"},
	{KMOD_SHIFT, "SHIFT"},
	{KMOD_CTRL, "CTRL"},
	{KMOD_ALT | KMOD_SHIFT, "ALT+SHIFT"},
	{KMOD_CTRL | KMOD_SHIFT, "CTRL+SHIFT"},
	{KMOD_ALT | KMOD_CTRL, "ALT+CTRL"},
	{KMOD_ALT | KMOD_SHIFT | KMOD_SHIFT, "CTRL+ALT+SHIFT"},
	{0, nullptr}
};

const char *WindowedApp::getModifierName(int16_t modifier) const {
	if (modifier == 0) {
		return nullptr;
	}
	for (int i = 0; i < lengthof(MODIFIERMAPPING); ++i) {
		if (MODIFIERMAPPING[i].modifier == modifier) {
			return MODIFIERMAPPING[i].name;
		}
	}
	return "<unknown>";
}

std::string WindowedApp::getKeyBindingsString(const char *cmd) const {
	int16_t modifier;
	int32_t key;
	if (!resolveKeyBindings(cmd, &modifier, &key)) {
		return "";
	}
	const char *name = SDL_GetKeyName((SDL_Keycode)key);
	if (modifier <= 0) {
		return name;
	}
	const char *modifierName = getModifierName(modifier);
	return core::string::format("%s+%s", modifierName, name);
}

bool WindowedApp::loadKeyBindings(const std::string& filename) {
	const std::string& bindings = filesystem()->load(filename);
	if (bindings.empty()) {
		return false;
	}
	Log::info("Load key bindings from %s", filename.c_str());
	const util::KeybindingParser p(bindings);
	_bindings.insert(p.getBindings().begin(), p.getBindings().end());
	return true;
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

	if (!loadKeyBindings()) {
		Log::error("failed to init the global keybindings");
	}
	loadKeyBindings(_appname + "-keybindings.cfg");

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
	const int numDisplays = (std::max)(0, SDL_GetNumVideoDisplays());
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
			_dpiFactor = ddpi / 96.0;
		}
		Log::info("Display %i: %i:%i x %i:%i (dpi: %f, h: %f, v: %f)", i, dr.x, dr.y, dr.w, dr.h, ddpi, hdpi, vdpi);
	}

	int width = core::Var::get(cfg::ClientWindowWidth, displayMode.w)->intVal();
	int height = core::Var::get(cfg::ClientWindowHeight, displayMode.h)->intVal();

	video::setup();

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
	SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
#ifdef SDL_HINT_MOUSE_DOUBLE_CLICK_TIME
	SDL_SetHint(SDL_HINT_MOUSE_DOUBLE_CLICK_TIME, "500");
#endif
#ifdef SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS
	SDL_SetHint(SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS, "32");
#endif

	const bool fullscreen = core::Var::getSafe(cfg::ClientFullscreen)->boolVal();

	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
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
		SDL_SetWindowGrab(_window, SDL_TRUE);
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
	int _width, _height;
	SDL_GetWindowSize(_window, &_width, &_height);
	_dimension = glm::ivec2(_width, _height);
	_aspect = _width / static_cast<float>(_height);

	video::init();
	video::viewport(0, 0, _width, _height);

	core_trace_gl_init();

	return state;
}

core::AppState WindowedApp::onConstruct() {
	core::AppState state = Super::onConstruct();
	core::Var::get(cfg::ClientMultiSampleBuffers, "0");
	core::Var::get(cfg::ClientMultiSampleSamples, "0");
	core::Var::get(cfg::ClientFullscreen, "true");
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

	core::Command::registerCommand("bindlist", [this] (const core::CmdArgs& args) {
		for (util::BindMap::const_iterator i = _bindings.begin(); i != _bindings.end(); ++i) {
			const util::CommandModifierPair& pair = i->second;
			const std::string& command = pair.command;
			const std::string& keyBinding = getKeyBindingsString(command.c_str());
			Log::info("%-25s %s", keyBinding.c_str(), command.c_str());
		}
	}).setHelp("Show all known key bindings");

	core::Command::registerCommand("bind", [this] (const core::CmdArgs& args) {
		if (args.size() != 2) {
			Log::error("Expected parameters: key+modifier command - got %i parameters", (int)args.size());
			return;
		}

		util::KeybindingParser p(args[0], args[1]);
		const util::BindMap& bindings = p.getBindings();
		for (util::BindMap::const_iterator i = bindings.begin(); i != bindings.end(); ++i) {
			const uint32_t key = i->first;
			const util::CommandModifierPair& pair = i->second;
			auto range = _bindings.equal_range(key);
			bool found = false;
			for (auto it = range.first; it != range.second; ++it) {
				if (it->second.modifier == pair.modifier) {
					it->second.command = pair.command;
					found = true;
					Log::info("Updated binding for key %s", args[0].c_str());
					break;
				}
			}
			if (!found) {
				_bindings.insert(std::make_pair(key, pair));
				Log::info("Added binding for key %s", args[0].c_str());
			}
		}
	}).setHelp("Bind a command to a key");

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

	std::string keybindings;

	for (util::BindMap::const_iterator i = _bindings.begin(); i != _bindings.end(); ++i) {
		const int32_t key = i->first;
		const util::CommandModifierPair& pair = i->second;
		const std::string keyName = core::string::toLower(SDL_GetKeyName(key));
		const int16_t modifier = pair.modifier;
		std::string modifierKey;
		if (modifier & KMOD_ALT) {
			modifierKey += "alt+";
		}
		if (modifier & KMOD_SHIFT) {
			modifierKey += "shift+";
		}
		if (modifier & KMOD_CTRL) {
			modifierKey += "ctrl+";
		}
		const std::string& command = pair.command;
		keybindings += modifierKey + keyName + " " + command + '\n';
	}
	Log::trace("%s", keybindings.c_str());
	filesystem()->write("keybindings.cfg", keybindings);

	return Super::onCleanup();
}

void WindowedApp::showCursor(bool show) {
	SDL_ShowCursor(show ? SDL_TRUE : SDL_FALSE);
}

void WindowedApp::centerMousePosition() {
	SDL_WarpMouseInWindow(_window, width() / 2, height() / 2);
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
