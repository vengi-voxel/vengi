/**
 * @file
 */

#include "../CameraPanel.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void CameraPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "create camera node")->TestFunc = [=](ImGuiTestContext *ctx) {
		const size_t beforeCamera = _sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Camera);
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemInputValue("##camera_props/Position/$$0/$$0", 0.0f);
		ctx->ItemInputValue("##camera_props/Position/$$0/$$1", 1.0f);
		ctx->ItemInputValue("##camera_props/Position/$$0/$$2", 2.0f);
		ctx->ItemClick("toolbar/###button1");
		const size_t afterCamera = _sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Camera);
		IM_CHECK_EQ(beforeCamera + 1, afterCamera);
	};

	IM_REGISTER_TEST(engine, testCategory(), "reset camera")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, "cameraresettest", voxel::Region(0, 31)));
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("toolbar/###button0");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "activate camera")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, "cameraactivate", voxel::Region(0, 31)));

		// add a camera node first
		const size_t before = _sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Camera);
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("toolbar/###button1"); // add camera
		ctx->Yield();
		IM_CHECK_EQ(_sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Camera), before + 1);

		// click activate camera button
		ctx->ItemClick("toolbar/###button2");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "projection combo")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, "cameraproj", voxel::Region(0, 31)));
		IM_CHECK(focusWindow(ctx, id));
		// the projection combo should exist
		const ImGuiTestItemInfo info = ctx->ItemInfo("###cameraproj", ImGuiTestOpFlags_NoError);
		IM_CHECK(info.ID != 0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "camera properties")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, "cameraprops", voxel::Region(0, 31)));
		IM_CHECK(focusWindow(ctx, id));

		// farplane and nearplane inputs should exist in the camera properties table
		const ImGuiTestItemInfo farInfo = ctx->ItemInfo("##camera_props/Farplane", ImGuiTestOpFlags_NoError);
		IM_CHECK(farInfo.ID != 0);

		const ImGuiTestItemInfo nearInfo = ctx->ItemInfo("##camera_props/Nearplane", ImGuiTestOpFlags_NoError);
		IM_CHECK(nearInfo.ID != 0);

		const ImGuiTestItemInfo fovInfo = ctx->ItemInfo("##camera_props/FOV", ImGuiTestOpFlags_NoError);
		IM_CHECK(fovInfo.ID != 0);
	};
}

} // namespace voxedit
