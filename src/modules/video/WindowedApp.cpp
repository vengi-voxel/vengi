/**
 * @file
 */

#include "WindowedApp.h"
#include "core/Common.h"
#include "core/Singleton.h"
#include "core/Var.h"
#include "Renderer.h"
#include "gl/GLVersion.h"
#include "Shader.h"
#include "core/Color.h"
#include "core/command/Command.h"
#include "core/Singleton.h"
#include "ShaderManager.h"
#include "util/KeybindingHandler.h"
#include <nfd.h>

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
#define sdlCheckError() checkSDLError(__FILE__, __LINE__, __PRETTY_FUNCTION__)
}

WindowedApp::WindowedApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, uint16_t traceport) :
		App(filesystem, eventBus, timeProvider, traceport), _dimension(-1) {
}

void WindowedApp::onAfterRunning() {
	core_trace_scoped(WindowedAppAfterRunning);
	video::endFrame(_window);
}

core::AppState WindowedApp::onRunning() {
	core::App::onRunning();

	for (KeyMapConstIter it = _keys.begin(); it != _keys.end(); ++it) {
		const int key = it->first;
		auto range = _bindings.equal_range(key);
		for (auto i = range.first; i != range.second; ++i) {
			const std::string& command = i->second.first;
			const int16_t modifier = i->second.second;
			if (it->second == modifier && command[0] == '+') {
				core_assert_always(1 == core::Command::execute(command + " true"));
				_keys[key] = modifier;
			}
		}
	}

	core_trace_scoped(WindowedAppOnRunning);
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			return core::AppState::Cleanup;
		default: {
			core_trace_scoped(WindowedAppEventHandler);
			const bool running = core::Singleton<io::EventHandler>::getInstance().handleEvent(event);
			if (!running) {
				return core::AppState::Cleanup;
			}
			break;
		}
		}
	}

	video::startFrame(_window, _rendererContext);
	video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);
	// TODO: maybe only do this every nth frame?
	core::Singleton<ShaderManager>::getInstance().update();

	return core::AppState::Running;
}

void WindowedApp::onWindowResize() {
	int _width, _height;
	SDL_GetWindowSize(_window, &_width, &_height);
	_aspect = _width / static_cast<float>(_height);
	_dimension = glm::ivec2(_width, _height);
	video::viewport(0, 0, _width, _height);
}

bool WindowedApp::onKeyRelease(int32_t key) {
	auto range = _bindings.equal_range(key);
	for (auto i = range.first; i != range.second; ++i) {
		const std::string& command = i->second.first;
		if (command[0] == '+' && _keys.erase(key) > 0) {
			core_assert_always(1 == core::Command::execute(command + " false"));
		}
	}
	return true;
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

	if (util::executeCommandsForBinding(_keys, _bindings, key, modifier)) {
		return true;
	}

	return false;
}

bool WindowedApp::loadKeyBindings(const std::string& filename) {
	const std::string& bindings = filesystem()->load(filename);
	if (bindings.empty()) {
		return false;
	}
	const util::KeybindingParser p(bindings);
	_bindings.insert(p.getBindings().begin(), p.getBindings().end());
	return true;
}

core::AppState WindowedApp::onInit() {
	core::AppState state = App::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		sdlCheckError();
		return core::AppState::Cleanup;
	}

	SDL_StopTextInput();

	if (!loadKeyBindings()) {
		Log::error("failed to init the global keybindings");
	}
	loadKeyBindings(_appname + "-keybindings.cfg");

	core::Singleton<io::EventHandler>::getInstance().registerObserver(this);

	SDL_DisplayMode displayMode;
	const int numDisplays = std::max(0, SDL_GetNumVideoDisplays());
	const int displayIndex = glm::clamp(core::Var::getSafe(cfg::ClientWindowDisplay)->intVal(), 0, numDisplays);
	SDL_GetDesktopDisplayMode(displayIndex, &displayMode);

	for (int i = 0; i < numDisplays; ++i) {
		SDL_Rect dr;
		if (SDL_GetDisplayBounds(i, &dr) == -1) {
			continue;
		}
		Log::info("Display %i: %i:%i x %i:%i", i, dr.x, dr.y, dr.w, dr.h);
	}

	int width = core::Var::get(cfg::ClientWindowWidth, displayMode.w)->intVal();
	int height = core::Var::get(cfg::ClientWindowHeight, displayMode.h)->intVal();

	video::setup();

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

	const bool fullscreen = core::Var::getSafe(cfg::ClientFullscreen)->boolVal();

	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
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

	_window = SDL_CreateWindow(_appname.c_str(), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex), width, height, flags);
	if (!_window) {
		sdlCheckError();
		return core::AppState::Cleanup;
	}

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
	core::AppState state = App::onConstruct();
	core::Var::get(cfg::ClientMultiSampleBuffers, "1");
	core::Var::get(cfg::ClientMultiSampleSamples, "4");
	core::Var::get(cfg::ClientFullscreen, "true");
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

	return state;
}

core::AppState WindowedApp::onCleanup() {
	core::Singleton<io::EventHandler>::getInstance().removeObserver(this);
	video::destroyContext(_rendererContext);
	SDL_DestroyWindow(_window);
	SDL_Quit();
	core_trace_gl_shutdown();

	std::string keybindings;

	for (util::BindMap::const_iterator i = _bindings.begin(); i != _bindings.end(); ++i) {
		const int32_t key = i->first;
		const util::CommandModifierPair& pair = i->second;
		const std::string keyName = core::string::toLower(SDL_GetKeyName(key));
		const int16_t modifier = pair.second;
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
		const std::string& command = pair.first;
		keybindings += modifierKey + keyName + " " + command + '\n';
	}
	Log::trace("%s", keybindings.c_str());
	filesystem()->write("keybindings.cfg", keybindings);

	return App::onCleanup();
}

std::string WindowedApp::fileDialog(OpenFileMode mode, const std::string& filter) {
	nfdchar_t *outPath = nullptr;
	nfdresult_t result;
	if (mode == OpenFileMode::Open) {
		result = NFD_OpenDialog(filter.c_str(), nullptr, &outPath);
	} else if (mode == OpenFileMode::Save) {
		result = NFD_SaveDialog(filter.c_str(), nullptr, &outPath);
	} else {
		result = NFD_PickFolder(nullptr, &outPath);
	}
	if (outPath && result == NFD_OKAY) {
		Log::info("Selected %s", outPath);
		std::string path = outPath;
		free(outPath);
		SDL_RaiseWindow(_window);
		SDL_SetWindowInputFocus(_window);
		return path;
	} else if (result == NFD_CANCEL) {
		Log::info("Cancel selection");
	} else if (result == NFD_ERROR) {
		const char *error = NFD_GetError();
		if (error == nullptr) {
			error = "Unknown";
		}
		Log::info("Error: %s", error);
	}
	free(outPath);
	SDL_RaiseWindow(_window);
	SDL_SetWindowInputFocus(_window);
	return "";
}

}
