/**
 * @file
 */

#pragma once

#include "app/tests/AbstractTest.h"
#include "core/ConfigVar.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/Singleton.h"
#include "video/Renderer.h"
#include "video/ShaderManager.h"
#include "voxel/SurfaceExtractor.h"
#include "core/sdl/SDLSystem.h"

namespace video {

// can be used for parameterized tests
// inherit from ::testing::WithParamInterface<ShaderVarState>
// and use GetParam() to get the values
// don't forget to do INSTANTIATE_TEST_CASE_P
struct ShaderVarState {
	bool clientShadowMap = true;
	bool clientDebugShadow = false;
};

inline ::std::ostream& operator<<(::std::ostream& os, const ShaderVarState& state) {
	return os << "state["
			<< "clientShadowMap(" << state.clientShadowMap << "), "
			<< "clientDebugShadow(" << state.clientDebugShadow << ")"
			<< "]";
}

class AbstractGLTest : public app::AbstractTest {
protected:
	SDL_Window *_window = nullptr;
	RendererContext _ctx = nullptr;

	void setShaderVars(const ShaderVarState& val) {
		core::Var::get(cfg::RenderCheckerBoard, "false", core::CV_SHADER);
		core::Var::get(cfg::ClientShadowMap, "1", core::CV_SHADER)->setVal(val.clientShadowMap);
		core::Var::get(cfg::ClientDebugShadow, "1", core::CV_SHADER)->setVal(val.clientDebugShadow);
		core::Var::get(cfg::ClientShadowMapSize, "128", core::CV_SHADER);
		core::Var::get(cfg::ClientGamma, "1.0", core::CV_SHADER);
		core::Var::get(cfg::ClientBloom, "false");
		core::Var::get(cfg::RenderCullBuffers, "false");
		core::Var::get(cfg::RenderCullNodes, "true");
		core::Var::get(cfg::RenderOutline, "false", core::CV_SHADER);
		core::Var::get(cfg::RenderNormals, "false", core::CV_SHADER);
		core::Var::get(cfg::ToneMapping, "0", core::CV_SHADER);
		core::Var::get(cfg::ClientDebugShadow, "false", core::CV_SHADER);
		core::Var::get(cfg::ClientDebugShadowMapCascade, "false", core::CV_SHADER);
		core::Var::get(cfg::VoxRenderMeshMode, core::string::toString((int)voxel::SurfaceExtractionType::Cubic), core::CV_SHADER);
		core::Singleton<ShaderManager>::getInstance().update();
	}

public:
	void SetUp() override {
		// 4.3 is the minimum version where compute shaders are supported
		core::Var::get(cfg::ClientOpenGLVersion, "4.3", core::CV_READONLY);
		core::Var::get(cfg::ClientMultiSampleBuffers, "0");
		core::Var::get(cfg::ClientMultiSampleSamples, "0");
		core::Var::get(cfg::ClientVSync, "false");
		core::Var::get(cfg::MaxAnisotropy, "1.0");
		core::Var::get(cfg::ClientDebugSeverity, "3");
		app::AbstractTest::SetUp();
#if defined(_WIN32) || defined(__CYGWIN__)
		GTEST_SKIP() << "Skipping because there are problems in the pipeline when running this headless";
#else
#if SDL_VERSION_ATLEAST(3, 2, 0)
		if (!SDL_Init(SDL_INIT_VIDEO)) {
#else
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
#endif
			GTEST_SKIP() << "Failed to initialize SDL video subsystem";
			return;
		}
		video::setup();
		video::construct();

#if SDL_VERSION_ATLEAST(3, 2, 0)
		SDL_PropertiesID props = SDL_CreateProperties();
		SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "test");
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 640);
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 480);
		SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
		_window = SDL_CreateWindowWithProperties(props);
		SDL_DestroyProperties(props);
#else
		_window = SDL_CreateWindow("test", 0, 0, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
#endif

		if (_window != nullptr) {
			_ctx = video::createContext(_window);
			if (_ctx == nullptr) {
				GTEST_SKIP() << "Failed to create context";
			} else if (!video::init(640, 480, 1.0f)) {
				GTEST_SKIP() << "Failed to init video context";
			}
		} else {
			GTEST_SKIP() << "Failed to create window";
		}
#endif
	}

	void TearDown() override {
		app::AbstractTest::TearDown();
		if (_ctx != nullptr) {
			video::destroyContext(_ctx);
			_ctx = nullptr;
		}
		if (_window != nullptr) {
			SDL_DestroyWindow(_window);
			_window = nullptr;
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
		if (IsSkipped()) {
			return;
		}
		setShaderVars(GetParam());
	}
};

}

#define VIDEO_SHADERTEST(testname)								\
	INSTANTIATE_TEST_SUITE_P(									\
		ShaderVars,												\
		testname,												\
		::testing::Values(										\
			video::ShaderVarState{true, true},					\
			video::ShaderVarState{true, false},					\
			video::ShaderVarState{false, false},				\
			video::ShaderVarState{false, true}					\
		)														\
	);
