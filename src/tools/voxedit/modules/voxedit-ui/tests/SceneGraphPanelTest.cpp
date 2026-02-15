/**
 * @file
 */

#include "../SceneGraphPanel.h"
#include "command/CommandHandler.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "../WindowTitles.h"

namespace voxedit {

static void contextMenuForNode(const SceneManagerPtr &sceneMgr, ImGuiTestContext *ctx, int nodeId, const char *uiId) {
	scenegraph::SceneGraphNode *modelNode = sceneMgr->sceneGraphModelNode(nodeId);
	IM_CHECK(modelNode != nullptr);
	const core::String uiNodeId = core::String::format("##nodelist/%s##%i", modelNode->name().c_str(), modelNode->id());
	// move to the node and open the context menu
	ctx->MouseMove(uiNodeId.c_str());
	ctx->MouseClick(ImGuiMouseButton_Right);
	const core::String clickId = core::String::format("//$FOCUSED/%s", uiId);
	ctx->MenuClick(clickId.c_str());
}

void SceneGraphPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "context menu")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(_sceneMgr->newScene(true, "scenegraphtest", voxel::Region(0, 31)));

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();

		// duplicate the node
		const int beforeDuplicate = sceneGraph.size(scenegraph::SceneGraphNodeType::Model);
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Duplicate");
		const int afterDuplicate = sceneGraph.size(scenegraph::SceneGraphNodeType::Model);
		IM_CHECK(afterDuplicate == beforeDuplicate + 1);

		// rename the node
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Rename");
		IM_CHECK(focusWindow(ctx, POPUP_TITLE_RENAME_NODE));
		ctx->ItemInputValue("Name", "automated ui test rename");
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id)); // back to the scene graph panel

		// create reference
		const int beforeReference = sceneGraph.size(scenegraph::SceneGraphNodeType::ModelReference);
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Create reference");
		const int afterReference = sceneGraph.size(scenegraph::SceneGraphNodeType::ModelReference);
		IM_CHECK(afterReference == beforeReference + 1);

		// delete the reference again
		const int beforeDelete = sceneGraph.size(scenegraph::SceneGraphNodeType::ModelReference);
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Delete");
		const int afterDelete = sceneGraph.size(scenegraph::SceneGraphNodeType::ModelReference);
		IM_CHECK(afterDelete == beforeDelete - 1);

		// merge all
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Merge all");
		const int afterMerge = sceneGraph.size(scenegraph::SceneGraphNodeType::Model);
		IM_CHECK(afterMerge == 1);
	};

	IM_REGISTER_TEST(engine, testCategory(), "context menu bake and stamp")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(_sceneMgr->newScene(true, "scenegraphbakestamp", voxel::Region(0, 31)));

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();

		// bake transform
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Bake transform");
		ctx->Yield();

		// use as stamp
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Use as stamp");
		ctx->Yield();

		// save the model node (modelsave saves directly to a file, no dialog)
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Save");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "context menu add nodes")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(_sceneMgr->newScene(true, "scenegraphaddnodes", voxel::Region(0, 31)));

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();

		// add new group via context menu
		const int groupsBefore = sceneGraph.size(scenegraph::SceneGraphNodeType::Group);
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Add new group");
		ctx->Yield();
		const int groupsAfter = sceneGraph.size(scenegraph::SceneGraphNodeType::Group);
		IM_CHECK_EQ(groupsAfter, groupsBefore + 1);

		// add new camera via context menu
		const int camerasBefore = sceneGraph.size(scenegraph::SceneGraphNodeType::Camera);
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Add new camera");
		ctx->Yield();
		const int camerasAfter = sceneGraph.size(scenegraph::SceneGraphNodeType::Camera);
		IM_CHECK_EQ(camerasAfter, camerasBefore + 1);

		// add new point via context menu
		const int pointsBefore = sceneGraph.size(scenegraph::SceneGraphNodeType::Point);
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Add new point");
		ctx->Yield();
		const int pointsAfter = sceneGraph.size(scenegraph::SceneGraphNodeType::Point);
		IM_CHECK_EQ(pointsAfter, pointsBefore + 1);
	};

	IM_REGISTER_TEST(engine, testCategory(), "context menu merge visible locked")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(_sceneMgr->newScene(true, "scenegraphmerge", voxel::Region(0, 31)));

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();

		// fill voxels so merging produces a valid region
		command::executeCommands("fill");
		ctx->Yield(3);

		// duplicate node to have multiple nodes
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Duplicate");
		IM_CHECK_EQ(sceneGraph.size(scenegraph::SceneGraphNodeType::Model), 2);

		// merge visible
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Merge visible");
		ctx->Yield();
		IM_CHECK_EQ(sceneGraph.size(scenegraph::SceneGraphNodeType::Model), 1);
	};

	IM_REGISTER_TEST(engine, testCategory(), "model node")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();

		const int before = sceneGraph.size(scenegraph::SceneGraphNodeType::Model);
		ctx->ItemClick("toolbar/###button0");
		ctx->Yield();

		IM_CHECK(focusWindow(ctx, POPUP_TITLE_MODEL_NODE_SETTINGS));
		ctx->ItemInputValue("##modelsettingsname", "automated ui test node");
		ctx->ItemClick("###Ok");
		ctx->Yield();

		const int after = sceneGraph.size(scenegraph::SceneGraphNodeType::Model);
		IM_CHECK(after == before + 1);
	};

	IM_REGISTER_TEST(engine, testCategory(), "group node")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();

		const int before = sceneGraph.size(scenegraph::SceneGraphNodeType::Group);
		ctx->ItemClick("toolbar/###button1");
		const int after = sceneGraph.size(scenegraph::SceneGraphNodeType::Group);
		IM_CHECK(after == before + 1);
	};

	IM_REGISTER_TEST(engine, testCategory(), "context menu hide show")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(_sceneMgr->newScene(true, "scenegraphhideshowtest", voxel::Region(0, 31)));

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();

		// duplicate node to have multiple nodes
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Duplicate");
		IM_CHECK_EQ(sceneGraph.size(scenegraph::SceneGraphNodeType::Model), 2);

		// hide others
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Hide others");
		ctx->Yield();

		// show all
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Show all");
		ctx->Yield();

		// hide all
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Hide all");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "context menu lock unlock")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(_sceneMgr->newScene(true, "scenegraphlockunlocktest", voxel::Region(0, 31)));

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();

		// lock all
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Lock all");
		ctx->Yield();

		// unlock all
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Unlock all");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "context menu center origin")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(_sceneMgr->newScene(true, "scenegraphcenterorigintest", voxel::Region(0, 31)));

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();

		// center origin
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Center origin");
		ctx->Yield();

		// center reference
		contextMenuForNode(_sceneMgr, ctx, sceneGraph.activeNode(), "Center reference");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "toolbar show hide all")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(_sceneMgr->newScene(true, "scenegraphtoolbartest", voxel::Region(0, 31)));

		// click show all and hide all toolbar buttons
		ctx->ItemClick("toolbar/###button4"); // show all
		ctx->Yield();
		ctx->ItemClick("toolbar/###button5"); // hide all
		ctx->Yield();
	};
}

} // namespace voxedit
