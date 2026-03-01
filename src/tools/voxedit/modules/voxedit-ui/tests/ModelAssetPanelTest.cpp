/**
 * @file
 */

#include "../ModelAssetPanel.h"
#include "voxedit-util/SceneManager.h"
#include "TestUtil.h"

namespace voxedit {

void ModelAssetPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "load remote collection")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();

		// wait for the remote collection to load (async network fetch)
		bool found = false;
		for (int i = 0; i < 300; ++i) {
			ctx->Yield();
			if (!_collectionMgr->voxelFilesMap().empty()) {
				found = true;
				break;
			}
		}
		if (!found) {
			ctx->LogWarning("Remote collection not available - skipping test");
			return;
		}

		ctx->ItemDoubleClick("**/Oasis");
		ctx->ItemDoubleClick("**/Vengi voxelized");
		ctx->Yield(10);

		const int modelSize = (int)_sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Model);

		// right-click the file and import via context menu
		ctx->MouseMove("**/flighthelmet-scale-300.qb");
		ctx->MouseClick(ImGuiMouseButton_Right);
		ctx->MenuClick("//$FOCUSED/Add to scene");

		// wait for the async import to complete
		bool imported = false;
		for (int i = 0; i < 600; ++i) {
			ctx->Yield();
			if ((int)_sceneMgr->sceneGraph().size(scenegraph::SceneGraphNodeType::Model) > modelSize) {
				imported = true;
				break;
			}
		}
		IM_CHECK(imported);
	};
}

} // namespace voxedit
