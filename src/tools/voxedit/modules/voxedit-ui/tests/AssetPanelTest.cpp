/**
 * @file
 */

#include "../AssetPanel.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/SceneManager.h"
#include "core/StringUtil.h"
#include "TestUtil.h"

namespace voxedit {

void AssetPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	_collectionPanel.registerUITests(engine, id);

	IM_REGISTER_TEST(engine, testCategory(), "drag drop image")->TestFunc = [=](ImGuiTestContext *ctx) {
		if (_texturePool->cache().empty()) {
			ctx->LogInfo("No images found in asset panel");
			return;
		}
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
	};

	IM_REGISTER_TEST(engine, testCategory(), "load remote collection")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("##assetpaneltabs/Models");
		ctx->Yield();
		ctx->ItemDoubleClick("**/Oasis");
	};
}

} // namespace voxedit
