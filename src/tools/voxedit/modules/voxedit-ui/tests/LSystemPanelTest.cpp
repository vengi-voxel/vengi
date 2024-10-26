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
		if (core::Var::getSafe(cfg::VoxEditViewMode)->intVal() == (int)ViewMode::Simple) {
			return;
		}
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(_sceneMgr->newScene(true, "lsystem", voxel::Region(0, 31)));
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		const voxel::RawVolume *volume = _sceneMgr->volume(activeNode);
		IM_CHECK(volume != nullptr);
		_sceneMgr->modifier().setReferencePosition(volume->region().getLowerCenter());

		ctx->ItemClick("###Ok");
		IM_CHECK(voxelutil::visitVolume(*volume, voxelutil::EmptyVisitor(), voxelutil::SkipEmpty()) > 0);
	};
}

} // namespace voxedit
