/**
 * @file
 */

#include "../MementoPanel.h"
#include "TestUtil.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/Voxel.h"

namespace voxedit {

void MementoPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "undo redo navigation")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		const memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
		const int initialPos = mementoHandler.statePosition();
		const int initialSize = (int)mementoHandler.stateSize();
		IM_CHECK(initialSize > 0);

		// modify the scene to create undo states
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *model = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(model != nullptr);
		IM_CHECK(setVoxel(_sceneMgr, model, glm::ivec3(1, 1, 1), voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		IM_CHECK(setVoxel(_sceneMgr, model, glm::ivec3(2, 2, 2), voxel::createVoxel(voxel::VoxelType::Generic, 2)));

		// verify we can undo
		IM_CHECK(mementoHandler.canUndo());
		_sceneMgr->undo();
		ctx->Yield();
		IM_CHECK(mementoHandler.canRedo());

		// redo again
		_sceneMgr->redo();
		ctx->Yield();

		// now check the panel is visible and focusable
		IM_CHECK(focusWindow(ctx, id));
	};

	IM_REGISTER_TEST(engine, testCategory(), "state list display")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));

		// create some undo states
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *model = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(model != nullptr);
		IM_CHECK(setVoxel(_sceneMgr, model, glm::ivec3(0, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		IM_CHECK(setVoxel(_sceneMgr, model, glm::ivec3(1, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 2)));
		IM_CHECK(setVoxel(_sceneMgr, model, glm::ivec3(2, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 3)));

		const memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
		IM_CHECK((int)mementoHandler.stateSize() >= 3);

		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();

		// undo twice and verify position changed
		const int posBeforeUndo = mementoHandler.statePosition();
		_sceneMgr->undo();
		_sceneMgr->undo();
		ctx->Yield();
		const int posAfterUndo = mementoHandler.statePosition();
		IM_CHECK(posAfterUndo < posBeforeUndo);

		// redo once
		_sceneMgr->redo();
		ctx->Yield();
		const int posAfterRedo = mementoHandler.statePosition();
		IM_CHECK(posAfterRedo > posAfterUndo);
	};

	IM_REGISTER_TEST(engine, testCategory(), "click history entry")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));

		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		scenegraph::SceneGraphNode *model = _sceneMgr->sceneGraphModelNode(activeNode);
		IM_CHECK(model != nullptr);
		IM_CHECK(setVoxel(_sceneMgr, model, glm::ivec3(0, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		IM_CHECK(setVoxel(_sceneMgr, model, glm::ivec3(1, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 2)));

		const memento::MementoHandler &mementoHandler = _sceneMgr->mementoHandler();
		const int posAtEnd = mementoHandler.statePosition();
		IM_CHECK(posAtEnd >= 2);

		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();

		// verify the history listbox exists and has content
		const ImGuiTestItemInfo listbox = ctx->ItemInfo("##history-actions", ImGuiTestOpFlags_NoError);
		IM_CHECK(listbox.ID != 0);

		// click the listbox near the top to select an earlier state
		ImVec2 clickPos = listbox.RectFull.Min;
		clickPos.x += 10.0f;
		clickPos.y += 10.0f;
		ctx->MouseMoveToPos(clickPos);
		ctx->MouseClick();
		ctx->Yield();
		IM_CHECK(mementoHandler.statePosition() < posAtEnd);
	};
}

} // namespace voxedit
