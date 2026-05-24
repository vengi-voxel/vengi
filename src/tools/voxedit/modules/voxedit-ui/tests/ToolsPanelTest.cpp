/**
 * @file
 */

#include "../ToolsPanel.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "scenegraph/SceneGraphNode.h"
#include "TestUtil.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

void ToolsPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "scenetools")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));
		int buttonIdx = 0;
		for (;;) {
			const core::String &btnid = core::String::format("toolbar/###button%i", buttonIdx);
			if (ctx->ItemInfo(btnid.c_str(), ImGuiTestOpFlags_NoError).ID == 0) {
				break;
			}
			ctx->LogInfo("Found button %i", buttonIdx);
			ctx->ItemClick(btnid.c_str());
			++buttonIdx;
		}
		IM_CHECK(buttonIdx > 0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "edittools")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportEditMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));
		int buttonIdx = 0;
		for (;;) {
			const core::String &btnid = core::String::format("toolbar/###button%i", buttonIdx);
			if (ctx->ItemInfo(btnid.c_str(), ImGuiTestOpFlags_NoError).ID == 0) {
				break;
			}
			ctx->LogInfo("Found button %i", buttonIdx);
			ctx->ItemClick(btnid.c_str());
			++buttonIdx;
		}
		IM_CHECK(buttonIdx > 0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "crop button")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportEditMode(ctx, _app));

		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK(setVoxel(_sceneMgr, node, glm::ivec3(5, 5, 5), voxel::createVoxel(voxel::VoxelType::Generic, 1)));

		const voxel::Region regionBefore = node->region();
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("toolbar/###button0"); // crop
		ctx->Yield(3);

		node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK(node->region().getWidthInVoxels() < regionBefore.getWidthInVoxels());
	};

	IM_REGISTER_TEST(engine, testCategory(), "fillhollow button")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportEditMode(ctx, _app));

		// fill + hollow to create a shell, then click fillhollow button
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("toolbar/###button11"); // fill
		ctx->Yield(3);
		ctx->ItemClick("toolbar/###button8"); // hollow
		ctx->Yield(3);

		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		const int countAfterHollow = voxelutil::countVoxels(*node->volume());

		ctx->ItemClick("toolbar/###button7"); // fillhollow
		ctx->Yield(3);

		node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK(voxelutil::countVoxels(*node->volume()) > countAfterHollow);
	};

	IM_REGISTER_TEST(engine, testCategory(), "hollow button")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportEditMode(ctx, _app));

		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("toolbar/###button11"); // fill
		ctx->Yield(3);

		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		const int countBefore = voxelutil::countVoxels(*node->volume());

		ctx->ItemClick("toolbar/###button8"); // hollow
		ctx->Yield(3);

		node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK(voxelutil::countVoxels(*node->volume()) < countBefore);
	};

	IM_REGISTER_TEST(engine, testCategory(), "rotate buttons")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportEditMode(ctx, _app));

		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK(setVoxel(_sceneMgr, node, glm::ivec3(0, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		IM_CHECK(setVoxel(_sceneMgr, node, glm::ivec3(1, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 1)));

		IM_CHECK(focusWindow(ctx, id));
		// click the X rotate button inside the "Rotate on axis" section
		ctx->ItemClick("##rotatevolumeonaxis/X###X");
		ctx->Yield(3);

		node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK_EQ(voxelutil::countVoxels(*node->volume()), 2);
	};

	IM_REGISTER_TEST(engine, testCategory(), "flip buttons")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportEditMode(ctx, _app));

		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK(setVoxel(_sceneMgr, node, glm::ivec3(0, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 1)));

		IM_CHECK(focusWindow(ctx, id));
		// click the X flip button inside the "Flip on axis" section
		ctx->ItemClick("##flipvolumeonaxis/X###X");
		ctx->Yield(3);

		node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK_EQ(voxelutil::countVoxels(*node->volume()), 1);
	};

	IM_REGISTER_TEST(engine, testCategory(), "move button")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportEditMode(ctx, _app));

		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK(setVoxel(_sceneMgr, node, glm::ivec3(0, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 1)));

		IM_CHECK(focusWindow(ctx, id));
		// set X translate to 1 via the input field, then click Move
		ctx->ItemInputValue("##movevoxels/X", 1);
		ctx->ItemClick("##movevoxels/Move###Move");
		ctx->Yield(3);

		node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK(node->volume()->voxel(0, 0, 0).getMaterial() == voxel::VoxelType::Air);
	};

	IM_REGISTER_TEST(engine, testCategory(), "cursor inputs")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportEditMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		ctx->ItemInputValue("##cursor/##cursorx", 5);
		ctx->Yield(3);
		IM_CHECK_EQ(_sceneMgr->modifier().cursorPosition().x, 5);
	};

	IM_REGISTER_TEST(engine, testCategory(), "split objects button")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportEditMode(ctx, _app));

		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		// place two separate voxel clusters
		IM_CHECK(setVoxel(_sceneMgr, node, glm::ivec3(0, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		IM_CHECK(setVoxel(_sceneMgr, node, glm::ivec3(20, 20, 20), voxel::createVoxel(voxel::VoxelType::Generic, 1)));

		const int modelsBefore = (int)_sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Model);
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("toolbar/###button2"); // splitobjects
		ctx->Yield(3);
		IM_CHECK((int)_sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Model) > modelsBefore);
	};

	IM_REGISTER_TEST(engine, testCategory(), "scale down button")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportEditMode(ctx, _app));

		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("toolbar/###button11"); // fill
		ctx->Yield(3);

		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		const int widthBefore = node->region().getWidthInVoxels();

		ctx->ItemClick("toolbar/###button5"); // scaledown
		ctx->Yield(3);

		node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK(node->region().getWidthInVoxels() < widthBefore);
	};

	IM_REGISTER_TEST(engine, testCategory(), "scale up button")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportEditMode(ctx, _app));

		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("toolbar/###button11"); // fill
		ctx->Yield(3);

		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		const int widthBefore = node->region().getWidthInVoxels();

		ctx->ItemClick("toolbar/###button6"); // scaleup
		ctx->Yield(3);

		node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK(node->region().getWidthInVoxels() > widthBefore);
	};

	IM_REGISTER_TEST(engine, testCategory(), "gizmo settings")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportEditMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		// ensure the model gizmo is enabled so the checkboxes are not disabled
		_showGizmoModel->setVal(true);
		ctx->Yield();

		const bool snapBefore = core::getVar(cfg::VoxEditGizmoSnap)->boolVal();
		ctx->ItemClick("Snap to grid###Snap to grid");
		ctx->Yield();
		IM_CHECK(core::getVar(cfg::VoxEditGizmoSnap)->boolVal() != snapBefore);
		ctx->ItemClick("Snap to grid###Snap to grid");
		ctx->Yield();
		IM_CHECK(core::getVar(cfg::VoxEditGizmoSnap)->boolVal() == snapBefore);
	};

	IM_REGISTER_TEST(engine, testCategory(), "color to model button")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportEditMode(ctx, _app));

		// place voxels with two different colors
		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(nodeId);
		IM_CHECK(node != nullptr);
		IM_CHECK(setVoxel(_sceneMgr, node, glm::ivec3(0, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		IM_CHECK(setVoxel(_sceneMgr, node, glm::ivec3(1, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 2)));

		_sceneMgr->modifier().setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 1));

		const int modelsBefore = (int)_sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Model);
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("toolbar/###button4"); // colortomodel
		ctx->Yield(3);
		IM_CHECK((int)_sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Model) > modelsBefore);
	};

	IM_REGISTER_TEST(engine, testCategory(), "cursor details slider")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportEditMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		const int before = _cursorDetails->intVal();
		changeSlider(ctx, "##cursor/Cursor details", false);
		ctx->Yield();
		IM_CHECK(_cursorDetails->intVal() != before || _cursorDetails->intVal() == _cursorDetails->intMaxValue());
	};

	IM_REGISTER_TEST(engine, testCategory(), "resize to selection")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(newFilledScene(ctx, _sceneMgr, "toolsresizesel", voxel::Region(0, 31)));
		IM_CHECK(activateViewportEditMode(ctx, _app));

		// select a sub-region
		command::executeCommands("select all");
		ctx->Yield(3);

		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		IM_CHECK(_sceneMgr->hasSelection(nodeId));

		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("toolbar/###button1"); // resizetoselection
		ctx->Yield(3);
	};
}

} // namespace voxedit
