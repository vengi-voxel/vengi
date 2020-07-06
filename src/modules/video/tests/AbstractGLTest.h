/**
 * @file
 */

#pragma once

#include "core/tests/AbstractTest.h"
#include "core/GameConfig.h"
#include "core/Var.h"
#include "core/Singleton.h"
#include "video/Renderer.h"
#include "video/ShaderManager.h"
#include <SDL.h>

namespace video {

// can be used for parameterized tests
// inherit from ::testing::WithParamInterface<ShaderVarState>
// and use GetParam() to get the values
// don't forget to do INSTANTIATE_TEST_CASE_P
struct ShaderVarState {
	bool clientFog = true;
	bool clientShadowMap = true;
	bool clientWater = true;
	bool clientDebugShadow = false;
};

inline ::std::ostream& operator<<(::std::ostream& os, const ShaderVarState& state) {
	return os << "state["
			<< "clientFog(" << state.clientFog << "), "
			<< "clientShadowMap(" << state.clientShadowMap << "), "
			<< "clientWater(" << state.clientWater << "), "
			<< "clientDebugShadow(" << state.clientDebugShadow << ")"
			<< "]";
}

class AbstractGLTest : public core::AbstractTest {
protected:
	SDL_Window *_window = nullptr;
	RendererContext _ctx = nullptr;
	bool _supported = true;

	void setShaderVars(const ShaderVarState& val) {
		core::Var::get(cfg::ClientFog, "1", core::CV_SHADER)->setVal(val.clientFog);
		core::Var::get(cfg::ClientShadowMap, "1", core::CV_SHADER)->setVal(val.clientShadowMap);
		core::Var::get(cfg::ClientWater, "1", core::CV_SHADER)->setVal(val.clientWater);
		core::Var::get(cfg::ClientDebugShadow, "1", core::CV_SHADER)->setVal(val.clientDebugShadow);
		core::Var::get(cfg::ClientGamma, "2.2", core::CV_SHADER);
		core::Var::get(cfg::RenderOutline, "false", core::CV_SHADER);
		core::Var::get(cfg::ClientDebugShadow, "false", core::CV_SHADER);
		core::Var::get(cfg::ClientDebugShadowMapCascade, "false", core::CV_SHADER);
		core::Singleton<ShaderManager>::getInstance().update();
	}

public:
	void SetUp() override {
		// 4.3 is the minimum version where compute shaders are supported
		core::Var::get(cfg::ClientOpenGLVersion, "4.3", core::CV_READONLY);
		core::Var::get(cfg::ClientMultiSampleBuffers, "0");
		core::Var::get(cfg::ClientMultiSampleSamples, "0");
		core::Var::get(cfg::ClientVSync, "false");
		core::AbstractTest::SetUp();
		SDL_Init(SDL_INIT_VIDEO);
		video::setup();
		_window = SDL_CreateWindow("test", 0, 0, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
		if (_window != nullptr) {
			_ctx = video::createContext(_window);
			_supported = _ctx != nullptr && video::init(640, 480, 1.0f);
		} else {
			_supported = false;
		}
	}

	void TearDown() override {
		core::AbstractTest::TearDown();
		if (_ctx != nullptr) {
			video::destroyContext(_ctx);
		}
		if (_window != nullptr) {
			SDL_DestroyWindow(_window);
		}
		SDL_Quit();
	}
};

class AbstractShaderTest :
		public AbstractGLTest,
		public ::testing::WithParamInterface<video::ShaderVarState> {
public:
	void SetUp() override {
		AbstractGLTest::SetUp();
		setShaderVars(GetParam());
	}
};

}

#define VIDEO_SHADERTEST(testname)								\
	INSTANTIATE_TEST_SUITE_P(									\
		ShaderVars,												\
		testname,												\
		::testing::Values(										\
			video::ShaderVarState{true, true, true, true},		\
			video::ShaderVarState{true, true, false, false},	\
			video::ShaderVarState{true, true, true, false},		\
			video::ShaderVarState{true, false, false, false},	\
			video::ShaderVarState{true, false, false, true},	\
			video::ShaderVarState{true, false, true, true},		\
			video::ShaderVarState{false, false, false, false},	\
			video::ShaderVarState{false, true, false, false},	\
			video::ShaderVarState{false, true, true, false},	\
			video::ShaderVarState{false, true, true, true},		\
			video::ShaderVarState{false, false, true, false},	\
			video::ShaderVarState{false, false, true, true},	\
			video::ShaderVarState{false, false, false, true}	\
		)														\
	);
