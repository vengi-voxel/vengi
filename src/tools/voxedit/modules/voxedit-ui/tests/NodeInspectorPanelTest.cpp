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
		ctx->Yield();
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

	IM_REGISTER_TEST(engine, testCategory(), "region size add reset")->TestFunc = [=](ImGuiTestContext *ctx) {
		util::ScopedVarChange scoped(cfg::VoxEditRegionSizes, "");
		IM_CHECK(activateViewportEditMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		ctx->MenuClick("Options/Region sizes/Add");
		ctx->Yield(3);
		IM_CHECK(!_validRegionSizes.empty());
	};

	IM_REGISTER_TEST(engine, testCategory(), "scene options")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		IM_CHECK(toggleMenuCheckbox(ctx, "Options/Auto Keyframe", cfg::VoxEditAutoKeyFrame));
		IM_CHECK(toggleMenuCheckbox(ctx, "Options/Update children", cfg::VoxEditTransformUpdateChildren));
		IM_CHECK(toggleMenuCheckbox(ctx, "Options/Local transforms", cfg::VoxEditLocalSpace));
	};

	IM_REGISTER_TEST(engine, testCategory(), "scene options auto keyframe")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		const int nodeId = sceneGraph.activeNode();
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId);
		IM_CHECK(node != nullptr);

		// enable auto keyframe
		core::VarPtr autoKf = core::getVar(cfg::VoxEditAutoKeyFrame);
		autoKf->setVal(true);

		// move to a frame that has no keyframe yet
		_sceneMgr->setCurrentFrame(10);
		ctx->Yield();
		const size_t keyframesBefore = node->keyFrames().size();

		// modify a transform value - this should auto-create a keyframe
		ctx->ItemInputValue("##node_props/Translation/$$1/$$0", 5.0f);
		ctx->Yield(3);

		IM_CHECK(node->keyFrames().size() > keyframesBefore);

		// restore
		autoKf->setVal(true);
	};

	IM_REGISTER_TEST(engine, testCategory(), "transform tools with verification")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		// set a non-zero translation
		ctx->ItemInputValue("##node_props/Translation/$$1/$$0", 10.0f);
		ctx->Yield();

		// reset transforms and verify translation is back to zero
		ctx->MenuClick("Tools/Reset transforms");
		ctx->Yield(3);

		ctx->MenuClick("Tools/Mirror X");
		ctx->Yield();

		ctx->MenuClick("Tools/Mirror Y");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "ik constraints")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(changeViewMode(ctx, ViewMode::Default));
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		ctx->ItemOpen("**/IK Constraints");
		ctx->Yield();

		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(node != nullptr);
		IM_CHECK(!node->hasIKConstraint());

		ctx->ItemClick("**/Enable IK");
		ctx->Yield();
		IM_CHECK(node->hasIKConstraint());

		ctx->ItemClick("**/Anchor");
		ctx->Yield();

		ctx->ItemClick("**/Visible");
		ctx->Yield();

		ctx->ItemOpen("**/Swing Limits");
		ctx->Yield();
		ctx->ItemClick("**/Add swing limit");
		ctx->Yield();

		ctx->ItemClick("**/Enable IK");
		ctx->Yield();
		IM_CHECK(!node->hasIKConstraint());
	};

	IM_REGISTER_TEST(engine, testCategory(), "interpolation settings")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		for (int i = 0; i < lengthof(scenegraph::InterpolationTypeStr); ++i) {
			const core::String path = core::String::format("Interpolation/%s", scenegraph::InterpolationTypeStr[i]);
			ctx->ComboClick(path.c_str());
			ctx->Yield();
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "color histogram")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(newFilledScene(ctx, _sceneMgr, "histogramtest", voxel::Region(0, 7)));
		IM_CHECK(activateViewportEditMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		ctx->ItemOpen("**/Color Histogram");
		ctx->Yield();

		ctx->ItemClick("**/Analyze");
		ctx->Yield(3);

		IM_CHECK(!_cachedHistogram.empty());
	};
}

} // namespace voxedit
