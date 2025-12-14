/**
 * @file
 */

#include "../LSystemPanel.h"
#include "../ViewMode.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

void LSystemPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "default rule")->TestFunc = [=](ImGuiTestContext *ctx) {
		if (!viewModeLSystemPanel(core::Var::getSafe(cfg::VoxEditViewMode)->intVal())) {
			return;
		}
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(_sceneMgr->newScene(true, "lsystem", voxel::Region(0, 31)));
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		const voxel::RawVolume *volume = _sceneMgr->volume(activeNode);
		IM_CHECK(volume != nullptr);

		ctx->ItemInputValue("Iterations", 1);
		ctx->ItemClick("###Ok");
		ctx->Yield(1); // give a chance to execute at least one step of the lsystem generation
		_sceneMgr->lsystemAbort();
		IM_CHECK(voxelutil::countVoxels(*volume) > 0);
	};
}

} // namespace voxedit
