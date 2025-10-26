/**
 * @file
 */

#include "../TreePanel.h"
#include "../ViewMode.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

void TreePanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "create tree")->TestFunc = [=](ImGuiTestContext *ctx) {
		if (!viewModeTreePanel(core::Var::getSafe(cfg::VoxEditViewMode)->intVal())) {
			return;
		}
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(_sceneMgr->newScene(true, "trees", voxel::Region(0, 31)));
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		const voxel::RawVolume *volume = _sceneMgr->volume(activeNode);
		IM_CHECK(volume != nullptr);
		_sceneMgr->modifier().setReferencePosition(volume->region().getLowerCenter());

		for (int i = 0; i < (int)voxelgenerator::TreeType::Max; ++i) {
			ctx->ItemClick("###Type");
			core::String name = core::String::format("//$FOCUSED/%s", treeTypeName(i));
			ctx->ItemClick(name.c_str());
			ctx->ItemClick("###Ok");
			IM_CHECK(voxelutil::countVoxels(*volume) > 0);
		}
	};
}

} // namespace voxedit
