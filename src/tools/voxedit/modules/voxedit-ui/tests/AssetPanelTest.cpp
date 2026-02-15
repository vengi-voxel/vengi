/**
 * @file
 */

#include "../AssetPanel.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/SceneManager.h"
#include "core/StringUtil.h"
#include "TestUtil.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

void AssetPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	_collectionPanel.registerUITests(engine, id);

	IM_REGISTER_TEST(engine, testCategory(), "drag drop image")->TestFunc = [=](ImGuiTestContext *ctx) {
		if (_texturePool->cache().empty()) {
			ctx->LogInfo("No images found in asset panel");
			return;
		}
		IM_CHECK(_sceneMgr->newScene(true, "image drag and drop", voxel::Region(0, 31)));
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		const voxel::RawVolume *volume = _sceneMgr->volume(activeNode);
		IM_CHECK(volume != nullptr);

		const int viewportId = viewportEditMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);
		const core::String viewPortId = core::String::format("//%s", Viewport::viewportId(viewportId).c_str());

		const size_t n = core_min(3, _texturePool->cache().size());
		for (size_t i = 0; i < n; ++i) {
			IM_CHECK(focusWindow(ctx, id));
			ctx->ItemClick("##assetpaneltabs/Images");
			const core::String srcRef = core::String::format("##assetpaneltabs/Images/%i", (int)i);
			ctx->ItemDragAndDrop(srcRef.c_str(), viewPortId.c_str());
		}
		IM_CHECK(voxelutil::countVoxels(*volume) > 0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "load remote collection")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("##assetpaneltabs/Models");
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
		// TODO: wait until it's loaded and import flighthelmet-scale-300.qb via context menu - add to scene
		// const int modelSize = _sceneMgr->sceneGraph().size();
		// assetpanel\/##voxelfiles_134E7031/0x134E7031 [override]/Vengi voxelized (3)##Vengi voxelized/2/flighthelmet-scale-300.qb
		// const int modelSizeAfterImport = _sceneMgr->sceneGraph().size();
		// IM_CHECK_EQ(modelSizeAfterImport, modelSize + 1);
	};
}

} // namespace voxedit
