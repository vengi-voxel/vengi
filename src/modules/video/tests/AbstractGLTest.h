/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/GameConfig.h"
#include "core/Var.h"
#include "video/Renderer.h"
#include <SDL.h>

namespace video {

class AbstractGLTest : public core::AbstractTest {
protected:
	SDL_Window *_window = nullptr;
	RendererContext _ctx = nullptr;
public:
	void SetUp() override {
		core::Var::get(cfg::ClientOpenGLVersion, "3.2", core::CV_READONLY);
		core::Var::get(cfg::ClientMultiSampleBuffers, "1");
		core::Var::get(cfg::ClientMultiSampleSamples, "4");
		core::Var::get(cfg::ClientVSync, "false");
		core::AbstractTest::SetUp();
		SDL_Init(SDL_INIT_VIDEO);
		video::setup();
		_window = SDL_CreateWindow("test", 0, 0, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
		_ctx = video::createContext(_window);
		video::init();
	}

	void TearDown() override {
		core::AbstractTest::TearDown();
		video::destroyContext(_ctx);
		SDL_DestroyWindow(_window);
		SDL_Quit();
	}
};

}
