/**
 * @file
 */

#include "../ToolsPanel.h"
#include "core/StringUtil.h"
#include "scenegraph/SceneGraphNode.h"
#include "TestUtil.h"
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
		IM_CHECK(_sceneMgr->newScene(true, "toolscrop", voxel::Region(0, 31)));
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
		IM_CHECK(_sceneMgr->newScene(true, "toolsfillhollow", voxel::Region(0, 7)));
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
		IM_CHECK(_sceneMgr->newScene(true, "toolshollow", voxel::Region(0, 7)));
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
		IM_CHECK(_sceneMgr->newScene(true, "toolsrotate", voxel::Region(0, 7)));
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
		IM_CHECK(_sceneMgr->newScene(true, "toolsflip", voxel::Region(0, 7)));
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
		IM_CHECK(_sceneMgr->newScene(true, "toolsmove", voxel::Region(0, 31)));
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
		IM_CHECK(_sceneMgr->newScene(true, "toolscursor", voxel::Region(0, 31)));
		IM_CHECK(activateViewportEditMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		ctx->ItemInputValue("##cursor/##cursorx", 5);
		ctx->Yield(3);
		IM_CHECK_EQ(_sceneMgr->modifier().cursorPosition().x, 5);
	};
}

} // namespace voxedit
