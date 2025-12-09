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
}

} // namespace voxedit
