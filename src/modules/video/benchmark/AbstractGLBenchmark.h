/**
 * @file
 * GPU fixture for the existing Google Benchmark application infrastructure.
 */
#pragma once
#include "app/benchmark/AbstractBenchmark.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "core/sdl/SDLSystem.h"
#include "video/Renderer.h"

namespace video {
class AbstractGLBenchmark : public app::AbstractBenchmark {
	SDL_Window *_window = nullptr;
	video::RendererContext _context = nullptr;
	bool _ready = false;

protected:
	bool onInitApp() override {
		core::Var::registerVar(core::VarDef(cfg::ClientOpenGLVersion, "4.3", "", ""));
		core::Var::registerVar(core::VarDef(cfg::ClientMultiSampleBuffers, 0, "", ""));
		core::Var::registerVar(core::VarDef(cfg::ClientMultiSampleSamples, 0, "", ""));
		core::Var::registerVar(core::VarDef(cfg::ClientVSync, false, "", ""));
		core::Var::registerVar(core::VarDef(cfg::RenderMaxAnisotropy, 1.0f, "", ""));
		core::Var::registerVar(core::VarDef(cfg::ClientDebugSeverity, 3, "", ""));
		if (!SDL_Init(SDL_INIT_VIDEO))
			return true;
		video::setup();
		video::construct();
		_window = SDL_CreateWindow("GPU benchmark", 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
		if (_window)
			_context = video::createContext(_window);
		_ready = _context && video::init(640, 480, 1.0f);
		return true;
	}
	void onCleanupApp() override {
		if (_context)
			video::destroyContext(_context);
		if (_window)
			SDL_DestroyWindow(_window);
		_context = nullptr;
		_window = nullptr;
		_ready = false;
	}

public:
	bool ready() const {
		return _ready;
	}
};

} // namespace video
