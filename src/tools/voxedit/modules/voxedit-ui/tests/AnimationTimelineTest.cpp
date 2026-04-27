/**
 * @file
 */

#include "../AnimationTimeline.h"
#include "../WindowTitles.h"
#include "scenegraph/SceneGraph.h"
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

	IM_REGISTER_TEST(engine, testCategory(), "create select and move")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, "timelinemove", voxel::Region(0, 31)));
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId);
		IM_CHECK(node != nullptr);

		const size_t initialKeyFrames = node->keyFrames().size();

		// move the frame selector to a non-zero frame
		const ImGuiID wrapperId = ctx->WindowInfo("##sequencer_child_wrapper").ID;
		ctx->SetRef(wrapperId);
		const ImGuiTestItemInfo frameSelector = ctx->ItemInfo("sequencer/currentframeselector");
		ctx->MouseMove(frameSelector.ID);
		ctx->MouseDragWithDelta({ImGui::Size(10.0f), 0.0f}, ImGuiMouseButton_Left);
		const scenegraph::FrameIndex firstFrame = _sceneMgr->currentFrame();
		IM_CHECK(firstFrame > 0);

		// add a keyframe at the current frame
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("###Add");
		IM_CHECK(node->keyFrames().size() == initialKeyFrames + 1);
		IM_CHECK(node->hasKeyFrameForFrame(firstFrame));

		// move the frame selector further to a different position
		ctx->SetRef(wrapperId);
		ctx->MouseMove(frameSelector.ID);
		ctx->MouseDragWithDelta({ImGui::Size(15.0f), 0.0f}, ImGuiMouseButton_Left);
		const scenegraph::FrameIndex secondFrame = _sceneMgr->currentFrame();
		IM_CHECK(secondFrame > firstFrame);

		// add another keyframe at the new position
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("###Add");
		IM_CHECK(node->keyFrames().size() == initialKeyFrames + 2);
		IM_CHECK(node->hasKeyFrameForFrame(secondFrame));

		// verify both keyframes still exist at the expected frames
		IM_CHECK(node->hasKeyFrameForFrame(firstFrame));
		IM_CHECK(node->hasKeyFrameForFrame(secondFrame));
	};

	IM_REGISTER_TEST(engine, testCategory(), "create select and delete")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, "timelinedelete", voxel::Region(0, 31)));
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		const int nodeId = _sceneMgr->sceneGraph().activeNode();
		const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(nodeId);
		IM_CHECK(node != nullptr);

		const size_t initialKeyFrames = node->keyFrames().size();

		// move the frame selector to a non-zero frame
		const ImGuiID wrapperId = ctx->WindowInfo("##sequencer_child_wrapper").ID;
		ctx->SetRef(wrapperId);
		const ImGuiTestItemInfo frameSelector = ctx->ItemInfo("sequencer/currentframeselector");
		ctx->MouseMove(frameSelector.ID);
		ctx->MouseDragWithDelta({ImGui::Size(10.0f), 0.0f}, ImGuiMouseButton_Left);
		const scenegraph::FrameIndex currentFrame = _sceneMgr->currentFrame();
		IM_CHECK(currentFrame > 0);

		// add a keyframe at the current frame
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("###Add");
		IM_CHECK(node->keyFrames().size() == initialKeyFrames + 1);
		IM_CHECK(node->hasKeyFrameForFrame(currentFrame));

		// delete the keyframe at the current frame
		ctx->ItemClick("###Delete");
		IM_CHECK(node->keyFrames().size() == initialKeyFrames);
		IM_CHECK(!node->hasKeyFrameForFrame(currentFrame));
	};

	IM_REGISTER_TEST(engine, testCategory(), "drag frame pointer")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, "timelinedrag", voxel::Region(0, 31)));
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));

		const ImGuiID wrapperId = ctx->WindowInfo("##sequencer_child_wrapper").ID;
		ctx->SetRef(wrapperId);

		// verify we start at frame 0 after new scene
		_sceneMgr->setCurrentFrame(0);
		ctx->Yield();
		IM_CHECK_EQ(_sceneMgr->currentFrame(), 0);

		// drag the frame pointer to the right
		const ImGuiTestItemInfo frameSelector = ctx->ItemInfo("sequencer/currentframeselector");
		ctx->MouseMove(frameSelector.ID);
		ctx->MouseDragWithDelta({ImGui::Size(10.0f), 0.0f}, ImGuiMouseButton_Left);
		const scenegraph::FrameIndex firstFrame = _sceneMgr->currentFrame();
		IM_CHECK(firstFrame > 0);

		// drag further to the right
		ctx->MouseMove(frameSelector.ID);
		ctx->MouseDragWithDelta({ImGui::Size(10.0f), 0.0f}, ImGuiMouseButton_Left);
		const scenegraph::FrameIndex secondFrame = _sceneMgr->currentFrame();
		IM_CHECK(secondFrame > firstFrame);
	};

	IM_REGISTER_TEST(engine, testCategory(), "add keyframe to all nodes")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, "timelineaddall", voxel::Region(0, 31)));
		IM_CHECK(activateViewportSceneMode(ctx, _app));

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		const int firstNodeId = sceneGraph.activeNode();
		const int secondNodeId = _sceneMgr->addModelChild("second", 8, 8, 8);
		IM_CHECK(secondNodeId != InvalidNodeId);
		ctx->Yield(2);

		const scenegraph::SceneGraphNode *firstNode = _sceneMgr->sceneGraphNode(firstNodeId);
		const scenegraph::SceneGraphNode *secondNode = _sceneMgr->sceneGraphNode(secondNodeId);
		IM_CHECK(firstNode != nullptr);
		IM_CHECK(secondNode != nullptr);

		const size_t firstBefore = firstNode->keyFrames().size();
		const size_t secondBefore = secondNode->keyFrames().size();

		// move the frame pointer to a non-zero frame
		IM_CHECK(focusWindow(ctx, id));
		const ImGuiID wrapperId = ctx->WindowInfo("##sequencer_child_wrapper").ID;
		ctx->SetRef(wrapperId);
		const ImGuiTestItemInfo frameSelector = ctx->ItemInfo("sequencer/currentframeselector");
		ctx->MouseMove(frameSelector.ID);
		ctx->MouseDragWithDelta({ImGui::Size(10.0f), 0.0f}, ImGuiMouseButton_Left);
		IM_CHECK(_sceneMgr->currentFrame() > 0);

		// click "Add all" to add keyframes to all nodes
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("###Add all");

		IM_CHECK(firstNode->keyFrames().size() == firstBefore + 1);
		IM_CHECK(secondNode->keyFrames().size() == secondBefore + 1);
	};

	IM_REGISTER_TEST(engine, testCategory(), "switch node from timeline")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, "timelineswitch", voxel::Region(0, 31)));
		IM_CHECK(activateViewportSceneMode(ctx, _app));

		scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		const int firstNodeId = sceneGraph.activeNode();

		// add a second model node
		const int secondNodeId = _sceneMgr->addModelChild("second node", 32, 32, 32);
		IM_CHECK(secondNodeId != InvalidNodeId);
		ctx->Yield(2);

		// ensure the first node is the active one
		_sceneMgr->nodeActivate(firstNodeId);
		ctx->Yield();
		IM_CHECK(sceneGraph.activeNode() == firstNodeId);

		// focus on the animation timeline and click on the second node's timeline entry
		IM_CHECK(focusWindow(ctx, id));
		const ImGuiID wrapperId = ctx->WindowInfo("##sequencer_child_wrapper").ID;
		ctx->SetRef(wrapperId);

		const scenegraph::SceneGraphNode *secondNode = _sceneMgr->sceneGraphNode(secondNodeId);
		IM_CHECK(secondNode != nullptr);

		// find the first node's timeline entry to use as an anchor for positioning
		const core::String firstLabel = core::String::format("sequencer/###node-%i", firstNodeId);
		const core::String secondLabel = core::String::format("sequencer/###node-%i", secondNodeId);
		const ImGuiTestItemInfo firstEntry = ctx->ItemInfo(firstLabel.c_str());
		// click at the position of the second node's timeline entry (just below the first one)
		ImVec2 clickPos = firstEntry.RectFull.GetCenter();
		clickPos.y += firstEntry.RectFull.GetHeight() + 1.0f;
		ctx->MouseMoveToPos(clickPos);
		ctx->MouseClick();
		ctx->Yield();

		// verify the active node changed to the second node
		IM_CHECK_EQ(sceneGraph.activeNode(), secondNodeId);
	};
}

} // namespace voxedit
