/**
 * @file
 */

#include "../Viewport.h"
#include "command/CommandHandler.h"
#include "util/VarUtil.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxelutil/VolumeVisitor.h"
#include "TestUtil.h"

namespace voxedit {

void Viewport::registerUITests(ImGuiTestEngine *engine, const char *) {
	IM_REGISTER_TEST(engine, testCategory(), "set voxel")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportEditMode(ctx, _app));
		const int viewportId = viewportEditMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(centerOnViewport(ctx, _sceneMgr, viewportId));
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *model = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(model != nullptr);
		const int cnt = voxelutil::visitVolumeParallel(*model->volume());
		IM_CHECK(cnt == 0);
		util::ScopedVarChange scoped(cfg::VoxEditGridsize, "1");
		ctx->Yield();
		executeViewportClick();
		IM_CHECK_EQ(1, voxelutil::visitVolumeParallel(*model->volume()));
	};

	// TODO: viewport menubar tests

#if 0
	IM_REGISTER_TEST(engine, testCategory(), "select node")->TestFunc = [=](ImGuiTestContext *ctx) {
		const int viewportId = viewportSceneMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);
		// TODO: a scene mode test to create another node and select a node
	};
#endif
}

} // namespace voxedit
