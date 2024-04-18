/**
 * @file
 */

#include "../SceneGraphPanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void SceneGraphPanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	IM_REGISTER_TEST(engine, testCategory(), "context menu")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, title));
		IM_CHECK(_sceneMgr->newScene(true, "scenegraphtest", voxel::Region(0, 31)));
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *modelNode = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(modelNode != nullptr);
		ImGuiTestItemInfo *info = ctx->ItemInfo("##nodelist");
		const core::String uiNodeId = core::string::format("##nodelist/root##0/%s##%i", modelNode->name().c_str(), modelNode->id());
		// move to the node and open the context menu
		ctx->MouseMove(uiNodeId.c_str());
		ctx->MouseClick(ImGuiMouseButton_Right);
	};

	// TODO: test for toolbar actions
	// TODO: test for details view with adding and removing properties
}

} // namespace voxedit
