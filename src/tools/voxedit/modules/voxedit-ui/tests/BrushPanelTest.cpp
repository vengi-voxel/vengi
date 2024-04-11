/**
 * @file
 */

#include "../BrushPanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void BrushPanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	IM_REGISTER_TEST(engine, testName(), "cycle brush types")->TestFunc = [=](ImGuiTestContext *ctx) {
		voxedit::ModifierFacade &modifier = _sceneMgr->modifier();
		// enable edit mode - TODO: find the correct viewport id and don't hardcode it
		ctx->ItemClick("//###viewport1");
		ctx->SetRef(title);
		IM_CHECK(focusWindow(ctx, title));
		for (int i = 0; i < (int)BrushType::Max; ++i) {
			const core::String id = core::String::format("brushes/###button%d", i);
			ctx->ItemClick(id.c_str());
			ctx->Yield();
			const BrushType brushType = modifier.brushType();
			IM_CHECK_EQ((int)brushType, i);
		}
	};
}

} // namespace voxedit
