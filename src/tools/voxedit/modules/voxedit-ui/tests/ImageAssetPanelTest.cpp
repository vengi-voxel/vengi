/**
 * @file
 */

#include "../ImageAssetPanel.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/SceneManager.h"
#include "TestUtil.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

void ImageAssetPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
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
			const core::String srcRef = core::String::format("##image-%i", (int)i);
			ctx->ItemDragAndDrop(srcRef.c_str(), viewPortId.c_str());
		}
		IM_CHECK(voxelutil::countVoxels(*volume) > 0);
	};
}

} // namespace voxedit
