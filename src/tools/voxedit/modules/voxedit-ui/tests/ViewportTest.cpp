/**
 * @file
 */

#include "../Viewport.h"
#include "command/CommandHandler.h"
#include "voxedit-util/SceneManager.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

void Viewport::registerUITests(ImGuiTestEngine *engine, const char *) {
	IM_REGISTER_TEST(engine, testCategory(), "set voxel")->TestFunc = [=](ImGuiTestContext *ctx) {
		if (isSceneMode()) {
			return;
		}
		IM_CHECK(focusWindow(ctx, _uiId.c_str()));
		IM_CHECK(_sceneMgr->newScene(true, "set voxel", voxel::Region(0, 31)));
		ImGuiWindow* window = ImGui::FindWindowByName(_uiId.c_str());
		IM_CHECK_SILENT(window != nullptr);
		ctx->MouseMoveToPos(window->Rect().GetCenter());
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *model = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(model != nullptr);
		const int cnt = voxelutil::visitVolume(*model->volume(), voxelutil::EmptyVisitor(), voxelutil::SkipEmpty());
		IM_CHECK(cnt == 0);
		command::executeCommands("+actionexecute 1 1;-actionexecute 1 1");
		IM_CHECK_EQ(1, voxelutil::visitVolume(*model->volume(), voxelutil::EmptyVisitor(), voxelutil::SkipEmpty()));
	};

	// TODO: viewport menubar tests

#if 0
	IM_REGISTER_TEST(engine, testCategory(), "select node")->TestFunc = [=](ImGuiTestContext *ctx) {
		if (!isSceneMode()) {
			return;
		}
		// TODO: a scene mode test to create another node and select a node
	};
#endif
}

} // namespace voxedit
