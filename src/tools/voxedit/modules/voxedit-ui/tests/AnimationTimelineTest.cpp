/**
 * @file
 */

#include "../AnimationTimeline.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/SceneManager.h"
#include "TestUtil.h"

namespace voxedit {

void AnimationTimeline::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "create keyframe")->TestFunc = [=](ImGuiTestContext *ctx) {
		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId);
		IM_CHECK(node != nullptr);

		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));
		const ImGuiID wrapperId = ctx->WindowInfo("##sequencer_child_wrapper").ID;
		ctx->SetRef(wrapperId);
		const ImGuiTestItemInfo frameSelector = ctx->ItemInfo("sequencer/currentframeselector");
		ctx->MouseMove(frameSelector.ID);
		ctx->MouseDragWithDelta({ImGui::Size(10.0f), 0.0f}, ImGuiMouseButton_Left);
		IM_CHECK(focusWindow(ctx, id));
		const size_t before = node->keyFrames().size();
		ctx->ItemClick("###Add");
		const size_t after = node->keyFrames().size();
		IM_CHECK(after == before + 1);
	};
}

} // namespace voxedit
