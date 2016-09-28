/**
 * @file
 */

#include "WindowedApp.h"
#include "core/Common.h"
#include "core/Singleton.h"
#include "core/Var.h"
#include "GLFunc.h"
#include "Shader.h"
#include "core/Color.h"
#include "core/Command.h"
#include "GLVersion.h"
#include "core/Remotery.h"
#include "core/Singleton.h"
#include "ShaderManager.h"

namespace video {

namespace {
inline void checkError(const char *file, unsigned int line, const char *function) {
	const char *error = SDL_GetError();
	if (*error != '\0') {
		Log::error("%s (%s:%i => %s)", error, file, line, function);
		SDL_ClearError();
	} else {
		Log::error("unknown error (%s:%i => %s)", file, line, function);
	}
}
#define sdlCheckError() checkError(__FILE__, __LINE__, __PRETTY_FUNCTION__)
}

WindowedApp::WindowedApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, uint16_t traceport) :
		App(filesystem, eventBus, traceport), _window(nullptr), _glcontext(nullptr), _dimension(-1), _aspect(1.0f) {
}

void WindowedApp::onAfterRunning() {
	core_trace_scoped(WindowedAppAfterRunning);
	SDL_GL_SwapWindow(_window);
}

core::AppState WindowedApp::onRunning() {
	App::onRunning();

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

	core_trace_gl_scoped(WindowedAppPrepareContext);
	SDL_GL_MakeCurrent(_window, _glcontext);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// TODO: maybe only do this every nth frame?
	core::Singleton<ShaderManager>::getInstance().update();

	return core::AppState::Running;
}

void WindowedApp::onWindowResize() {
	int _width, _height;
	SDL_GetWindowSize(_window, &_width, &_height);
	_aspect = _width / static_cast<float>(_height);
	_dimension = glm::ivec2(_width, _height);
	glViewport(0, 0, _width, _height);
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

	auto range = _bindings.equal_range(key);
	for (auto i = range.first; i != range.second; ++i) {
		const std::string& command = i->second.first;
		const int mod = i->second.second;
		if (mod == KMOD_NONE && modifier != 0 && modifier != KMOD_NUM) {
			continue;
		}
		if (mod != KMOD_NONE && !(modifier & mod)) {
			continue;
		}
		if (_keys.find(key) == _keys.end()) {
			if (command[0] == '+') {
				if (core::Command::execute(command + " true") == 1) {
					_keys[key] = modifier;
				}
			} else {
				core::Command::execute(command);
			}
		}
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
	if (state == core::AppState::Cleanup) {
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
	SDL_GetDesktopDisplayMode(0, &displayMode);
	const char *name = SDL_GetPixelFormatName(displayMode.format);
	int width = core::Var::get(cfg::ClientWindowWidth, displayMode.w)->intVal();
	int height = core::Var::get(cfg::ClientWindowHeight, displayMode.h)->intVal();

	const core::VarPtr& glVersion = core::Var::get(cfg::ClientOpenGLVersion, "3.1", core::CV_READONLY);
	int glMinor = 0, glMajor = 0;
	if (std::sscanf(glVersion->strVal().c_str(), "%3i.%3i", &glMajor, &glMinor) != 2) {
		glMajor = 3;
		glMinor = 0;
	}
	GLVersion glv(glMajor, glMinor);
	for (size_t i = 0; i < SDL_arraysize(GLVersions); ++i) {
		if (GLVersions[i].version == glv) {
			Shader::glslVersion = GLVersions[i].glslVersion;
		}
	}
	SDL_ClearError();
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	const core::VarPtr& multisampleBuffers = core::Var::get(cfg::ClientMultiSampleBuffers, "1");
	const core::VarPtr& multisampleSamples = core::Var::get(cfg::ClientMultiSampleSamples, "4");
	const core::VarPtr& deferred = core::Var::get(cfg::ClientDeferred, "false", core::CV_SHADER);
	core::Var::get(cfg::ClientShadowMap, "true", core::CV_SHADER);

	bool multisampling = multisampleSamples->intVal() > 0 && multisampleBuffers->intVal() > 0;
	if (!deferred->boolVal() && multisampling) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, multisampleBuffers->intVal());
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multisampleSamples->intVal());
	} else {
		multisampling = false;
	}
#ifdef DEBUG
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, glv.majorVersion);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, glv.minorVersion);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

	const bool fullscreen = core::Var::get(cfg::ClientFullscreen, "true")->boolVal();

	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
	if (fullscreen) {
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS;
	}

	const int videoDrivers = SDL_GetNumVideoDrivers();
	for (int i = 0; i < videoDrivers; ++i) {
		Log::info("available driver: %s", SDL_GetVideoDriver(i));
	}

	Log::info("driver: %s", SDL_GetCurrentVideoDriver());
	const int displays = SDL_GetNumVideoDisplays();
	Log::info("found %i displays", displays);
	if (fullscreen && displays > 1) {
		width = displayMode.w;
		height = displayMode.h;
		Log::info("use fake fullscreen for the first display: %i:%i", width, height);
	}

	_window = SDL_CreateWindow(_appname.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
	if (!_window) {
		sdlCheckError();
		return core::AppState::Cleanup;
	}

	_glcontext = SDL_GL_CreateContext(_window);

	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &glv.majorVersion);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &glv.minorVersion);
	Log::info("got gl context: %i.%i", glv.majorVersion, glv.minorVersion);

	const bool vsync = core::Var::get(cfg::ClientVSync, "false")->boolVal();
	if (vsync) {
		if (SDL_GL_SetSwapInterval(-1) == -1) {
			if (SDL_GL_SetSwapInterval(1) == -1) {
				Log::warn("Could not activate vsync: %s", SDL_GetError());
			}
		}
	} else {
		SDL_GL_SetSwapInterval(0);
	}
	if (SDL_GL_GetSwapInterval() == 0) {
		Log::info("Deactivated vsync");
	} else {
		Log::info("Activated vsync");
	}

	int buffers, samples;
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buffers);
	SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &samples);
	if (buffers == 0 || samples == 0) {
		Log::warn("Could not get FSAA context");
		multisampling = false;
	} else {
		Log::info("Got FSAA context with %i buffers and %i samples", buffers, samples);
	}

	SDL_DisableScreenSaver();

	if (SDL_SetWindowBrightness(_window, 1.0f) == -1) {
		sdlCheckError();
	}

	const bool grabMouse = false;
	if (grabMouse && (!fullscreen || displays > 1)) {
		SDL_SetWindowGrab(_window, SDL_TRUE);
	}

	int screen = 0;
	int modes = SDL_GetNumDisplayModes(screen);
	Log::info("possible display modes:");
	for (int i = 0; i < modes; i++) {
		SDL_GetDisplayMode(screen, i, &displayMode);
		name = SDL_GetPixelFormatName(displayMode.format);
		Log::info("%ix%i@%iHz %s", displayMode.w, displayMode.h, displayMode.refresh_rate, name);
	}

	core::Var::get(cfg::ClientGamma, "2.2", core::CV_SHADER);

	// some platforms may override or hardcode the resolution - so
	// we have to query it here to get the actual resolution
	int _width, _height;
	SDL_GetWindowSize(_window, &_width, &_height);
	_dimension = glm::ivec2(_width, _height);
	_aspect = _width / static_cast<float>(_height);

	ExtGLLoadFunctions();
	glViewport(0, 0, _width, _height);

	float lineWidth[2];
	glGetFloatv(GL_LINE_WIDTH_RANGE, lineWidth);
	Log::debug("GL_LINE_WIDTH_RANGE %f - %f", lineWidth[0], lineWidth[1]);

	if (multisampling) {
		glEnable(GL_MULTISAMPLE);
	}

	rmt_BindOpenGL();

	return state;
}

core::AppState WindowedApp::onConstruct() {
	core::AppState state = App::onConstruct();
	return state;
}

core::AppState WindowedApp::onCleanup() {
	core::Singleton<io::EventHandler>::getInstance().removeObserver(this);
	SDL_GL_DeleteContext(_glcontext);
	SDL_DestroyWindow(_window);
	SDL_Quit();
	rmt_UnbindOpenGL();

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
	Log::info("%s", keybindings.c_str());
	filesystem()->write("keybindings.cfg", keybindings);


	return App::onCleanup();
}

}
