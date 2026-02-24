/**
 * @file
 */

#include "voxedit-util/SceneRenderer.h"
#include "video/tests/AbstractGLTest.h"
#include "voxedit-util/Config.h"

namespace voxedit {

class SceneRendererTest : public video::AbstractGLTest {
private:
	using Super = video::AbstractGLTest;

protected:
	SceneRenderer _sceneRenderer;

	void SetUp() override {
		Super::SetUp();
		if (IsSkipped()) {
			return;
		}
		video::ShaderVarState state;
		setShaderVars(state);
		core::Var::registerVar(cfg::VoxEditShowgrid, "true");
		core::Var::registerVar(cfg::VoxEditShowlockedaxis, "true");
		core::Var::registerVar(cfg::VoxEditRendershadow, "true");
		core::Var::registerVar(cfg::VoxEditGridsize, "1");
		core::Var::registerVar(cfg::VoxEditPlaneSize, "0");
		core::Var::registerVar(cfg::VoxEditShowPlane, "false");
		core::Var::registerVar(cfg::VoxRenderMeshMode, core::string::toString((int)voxel::SurfaceExtractionType::Binary));
		core::Var::registerVar(cfg::VoxEditShowaabb, "");
		core::Var::registerVar(cfg::VoxEditShowBones, "");
		core::Var::registerVar(cfg::VoxEditGrayInactive, "");
		core::Var::registerVar(cfg::VoxEditHideInactive, "");
		_sceneRenderer.construct();
		ASSERT_TRUE(_sceneRenderer.init());
	};

	void TearDown() override {
		if (!IsSkipped()) {
			_sceneRenderer.shutdown();
		}
		Super::TearDown();
	}
};

TEST_F(SceneRendererTest, testInit) {
	// empty to test init and shutdown
}

} // namespace voxedit
