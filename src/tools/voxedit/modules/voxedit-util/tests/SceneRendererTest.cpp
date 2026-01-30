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
		core::Var::get(cfg::VoxEditShowgrid, "true");
		core::Var::get(cfg::VoxEditShowlockedaxis, "true");
		core::Var::get(cfg::VoxEditRendershadow, "true");
		core::Var::get(cfg::VoxEditGridsize, "1");
		core::Var::get(cfg::VoxEditPlaneSize, "0");
		core::Var::get(cfg::VoxEditShowPlane, "false");
		core::Var::get(cfg::VoxRenderMeshMode, core::string::toString((int)voxel::SurfaceExtractionType::Binary));
		core::Var::get(cfg::VoxEditShowaabb, "");
		core::Var::get(cfg::VoxEditShowBones, "");
		core::Var::get(cfg::VoxEditGrayInactive, "");
		core::Var::get(cfg::VoxEditHideInactive, "");
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
