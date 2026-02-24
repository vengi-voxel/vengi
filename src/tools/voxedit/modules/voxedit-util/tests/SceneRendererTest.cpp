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
		const core::VarDef voxEditShowgrid(cfg::VoxEditShowgrid, true, "", "");
		core::Var::registerVar(voxEditShowgrid);
		const core::VarDef voxEditShowlockedaxis(cfg::VoxEditShowlockedaxis, true, "", "");
		core::Var::registerVar(voxEditShowlockedaxis);
		const core::VarDef voxEditRendershadow(cfg::VoxEditRendershadow, true, "", "");
		core::Var::registerVar(voxEditRendershadow);
		const core::VarDef voxEditGridsize(cfg::VoxEditGridsize, 1, "", "");
		core::Var::registerVar(voxEditGridsize);
		const core::VarDef voxEditPlaneSize(cfg::VoxEditPlaneSize, 0, "", "");
		core::Var::registerVar(voxEditPlaneSize);
		const core::VarDef voxEditShowPlane(cfg::VoxEditShowPlane, false, "", "");
		core::Var::registerVar(voxEditShowPlane);
		const core::VarDef voxRenderMeshMode(cfg::VoxRenderMeshMode, (int)voxel::SurfaceExtractionType::Binary, "", "");
		core::Var::registerVar(voxRenderMeshMode);
		const core::VarDef voxEditShowaabb(cfg::VoxEditShowaabb, "", "", "");
		core::Var::registerVar(voxEditShowaabb);
		const core::VarDef voxEditShowBones(cfg::VoxEditShowBones, "", "", "");
		core::Var::registerVar(voxEditShowBones);
		const core::VarDef voxEditGrayInactive(cfg::VoxEditGrayInactive, "", "", "");
		core::Var::registerVar(voxEditGrayInactive);
		const core::VarDef voxEditHideInactive(cfg::VoxEditHideInactive, "", "", "");
		core::Var::registerVar(voxEditHideInactive);
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
