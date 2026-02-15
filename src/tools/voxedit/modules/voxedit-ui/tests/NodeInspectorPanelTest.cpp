/**
 * @file
 */

#include "../NodeInspectorPanel.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "TestUtil.h"

namespace voxedit {

void NodeInspectorPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "transform")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemInputValue("##node_props/Translation/$$1/$$0", 1.0f);
		ctx->ItemInputValue("##node_props/Translation/$$1/$$1", 2.0f);
		ctx->ItemInputValue("##node_props/Translation/$$1/$$2", 3.0f);

		ctx->ItemInputValue("##node_props/Rotation/$$2/$$0", 45.0f);
		ctx->ItemInputValue("##node_props/Rotation/$$2/$$1", 90.0f);
		ctx->ItemInputValue("##node_props/Rotation/$$2/$$2", 45.0f);

		ctx->ItemInputValue("##node_props/Scale/$$3/$$0", 2.0f);
		ctx->ItemInputValue("##node_props/Scale/$$3/$$1", 2.0f);
		ctx->ItemInputValue("##node_props/Scale/$$3/$$2", 2.0f);

		ctx->ItemInputValue("##node_props/Pivot/$$4/$$0", 1.0f);
		ctx->ItemInputValue("##node_props/Pivot/$$4/$$1", 1.0f);
		ctx->ItemInputValue("##node_props/Pivot/$$4/$$2", 1.0f);

		ctx->MenuClick("Tools/Reset transforms");
	};

	IM_REGISTER_TEST(engine, testCategory(), "sizes")->TestFunc = [=](ImGuiTestContext *ctx) {
		util::ScopedVarChange scoped(cfg::VoxEditRegionSizes, "3 3 3,2 2 2,1 1 1");
		IM_CHECK(activateViewportEditMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		ctx->ItemClick("toolbar/2x2x2##regionsize");
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *model = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(model != nullptr);
		const voxel::Region &region = model->region();
		IM_CHECK_EQ(2, region.getDimensionsInVoxels().x);
		IM_CHECK_EQ(2, region.getDimensionsInVoxels().y);
		IM_CHECK_EQ(2, region.getDimensionsInVoxels().z);
	};

	// scene mode options menu
	IM_REGISTER_TEST(engine, testCategory(), "scene options")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		// toggle auto keyframe option and check the result
		ctx->MenuClick("Options/Auto Keyframe");
		ctx->Yield();

		ctx->MenuClick("Options/Auto Keyframe");
		ctx->Yield();

		// toggle update children
		ctx->MenuClick("Options/Update children");
		ctx->Yield();

		ctx->MenuClick("Options/Update children");
		ctx->Yield();

		// toggle local transforms
		ctx->MenuClick("Options/Local transforms");
		ctx->Yield();

		ctx->MenuClick("Options/Local transforms");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "transform tools")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		ctx->MenuClick("Tools/Reset transforms");
		ctx->Yield();

		ctx->MenuClick("Tools/Mirror X");
		ctx->Yield();

		ctx->MenuClick("Tools/Mirror Y");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "ik constraints")->TestFunc = [=](ImGuiTestContext *ctx) {
		// IK constraints are only visible in animation view mode (Default includes animations)
		IM_CHECK(changeViewMode(ctx, ViewMode::Default));
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		// open the IK Constraints collapsing header
		ctx->ItemOpen("**/IK Constraints");
		ctx->Yield();

		// the active node should not have IK constraints yet
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(node != nullptr);
		IM_CHECK(!node->hasIKConstraint());

		// enable IK
		ctx->ItemClick("**/Enable IK");
		ctx->Yield();
		IM_CHECK(node->hasIKConstraint());

		// toggle anchor
		ctx->ItemClick("**/Anchor");
		ctx->Yield();

		// toggle visible
		ctx->ItemClick("**/Visible");
		ctx->Yield();

		// open swing limits header and add a swing limit
		ctx->ItemOpen("**/Swing Limits");
		ctx->Yield();
		ctx->ItemClick("**/Add swing limit");
		ctx->Yield();

		// disable IK again
		ctx->ItemClick("**/Enable IK");
		ctx->Yield();
		IM_CHECK(!node->hasIKConstraint());
	};
}

} // namespace voxedit
