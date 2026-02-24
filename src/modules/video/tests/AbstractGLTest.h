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
		const core::VarDef renderCheckerBoard(cfg::RenderCheckerBoard, false, "", "", core::CV_SHADER);
		core::Var::registerVar(renderCheckerBoard);
		const core::VarDef clientShadowMap(cfg::ClientShadowMap, 1, "", "", core::CV_SHADER);
		core::Var::registerVar(clientShadowMap)->setVal(val.clientShadowMap);
		const core::VarDef clientDebugShadow(cfg::ClientDebugShadow, 1, "", "", core::CV_SHADER);
		core::Var::registerVar(clientDebugShadow)->setVal(val.clientDebugShadow);
		const core::VarDef clientShadowMapSize(cfg::ClientShadowMapSize, 128, "", "", core::CV_SHADER);
		core::Var::registerVar(clientShadowMapSize);
		const core::VarDef clientGamma(cfg::ClientGamma, 1.0f, "", "", core::CV_SHADER);
		core::Var::registerVar(clientGamma);
		const core::VarDef clientBloom(cfg::ClientBloom, false, "", "");
		core::Var::registerVar(clientBloom);
		const core::VarDef renderCullBuffers(cfg::RenderCullBuffers, false, "", "");
		core::Var::registerVar(renderCullBuffers);
		const core::VarDef renderCullNodes(cfg::RenderCullNodes, true, "", "");
		core::Var::registerVar(renderCullNodes);
		const core::VarDef renderOutline(cfg::RenderOutline, false, "", "", core::CV_SHADER);
		core::Var::registerVar(renderOutline);
		const core::VarDef renderNormals(cfg::RenderNormals, false, "", "", core::CV_SHADER);
		core::Var::registerVar(renderNormals);
		const core::VarDef toneMapping(cfg::ToneMapping, 0, "", "", core::CV_SHADER);
		core::Var::registerVar(toneMapping);
		const core::VarDef clientDebugShadow2(cfg::ClientDebugShadow, false, "", "", core::CV_SHADER);
		core::Var::registerVar(clientDebugShadow2);
		const core::VarDef clientDebugShadowMapCascade(cfg::ClientDebugShadowMapCascade, false, "", "", core::CV_SHADER);
		core::Var::registerVar(clientDebugShadowMapCascade);
		const core::VarDef voxRenderMeshMode(cfg::VoxRenderMeshMode, (int)voxel::SurfaceExtractionType::Cubic, "", "", core::CV_SHADER);
		core::Var::registerVar(voxRenderMeshMode);
		core::Singleton<ShaderManager>::getInstance().update();
	}

public:
	void SetUp() override {
		// 4.3 is the minimum version where compute shaders are supported
		const core::VarDef clientOpenGLVersion(cfg::ClientOpenGLVersion, "4.3", "", "", core::CV_READONLY);
		core::Var::registerVar(clientOpenGLVersion);
		const core::VarDef clientMultiSampleBuffers(cfg::ClientMultiSampleBuffers, 0, "", "");
		core::Var::registerVar(clientMultiSampleBuffers);
		const core::VarDef clientMultiSampleSamples(cfg::ClientMultiSampleSamples, 0, "", "");
		core::Var::registerVar(clientMultiSampleSamples);
		const core::VarDef clientVSync(cfg::ClientVSync, false, "", "");
		core::Var::registerVar(clientVSync);
		const core::VarDef maxAnisotropy(cfg::MaxAnisotropy, 1.0f, "", "");
		core::Var::registerVar(maxAnisotropy);
		const core::VarDef clientDebugSeverity(cfg::ClientDebugSeverity, 3, "", "");
		core::Var::registerVar(clientDebugSeverity);
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
