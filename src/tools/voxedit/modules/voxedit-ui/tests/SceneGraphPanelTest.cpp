/**
 * @file
 */

#include "../SceneGraphPanel.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "../WindowTitles.h"

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

	IM_REGISTER_TEST(engine, testCategory(), "model node")->TestFunc = [=](ImGuiTestContext *ctx) {
		ImGuiContext& g = *ctx->UiContext;
		IM_CHECK(focusWindow(ctx, title));

		const int before = _sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Model);
		ctx->ItemClick("scenegraphtools/###button0");
		ctx->Yield();

		IM_CHECK(focusWindow(ctx, POPUP_TITLE_MODEL_NODE_SETTINGS));
		ctx->ItemInputValue("##modelsettingsname", "automated ui test node");
		ctx->ItemClick("###OK");
		ctx->Yield();

		const int after = _sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Model);
		IM_CHECK(after == before + 1);
	};

	IM_REGISTER_TEST(engine, testCategory(), "group node")->TestFunc = [=](ImGuiTestContext *ctx) {
		ImGuiContext& g = *ctx->UiContext;
		IM_CHECK(focusWindow(ctx, title));
		const int before = _sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Group);
		ctx->ItemClick("scenegraphtools/###button1");
		const int after = _sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Group);
		IM_CHECK(after == before + 1);
	};

	// TODO: test for details view with adding and removing properties
}

} // namespace voxedit
