/**
 * @file
 */

#include "../ToolsPanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void ToolsPanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	IM_REGISTER_TEST(engine, testCategory(), "toolbar")->TestFunc = [=](ImGuiTestContext *ctx) {
		ctx->SetRef(title);
		IM_CHECK(focusWindow(ctx, title));
		// edittools###button0
		// scenetools###button0
		ImGuiTestItemList items;
		bool editMode = ctx->ItemInfo("edittools/###button0", ImGuiTestOpFlags_NoError)->ID != 0;
		bool sceneMode = false;
		if (!editMode)
			sceneMode = ctx->ItemInfo("scenetools/###button0", ImGuiTestOpFlags_NoError)->ID != 0;
		IM_CHECK(editMode || sceneMode);
		int buttonIdx = 0;
		for (;;) {
			core::String id = core::string::format("%s/###button%i", editMode ? "edittools" : "scenetools", buttonIdx);
			if (ctx->ItemInfo(id.c_str(), ImGuiTestOpFlags_NoError)->ID == 0) {
				break;
			}
			ctx->LogInfo("Found button %i", buttonIdx);
			ctx->ItemClick(id.c_str());
			++buttonIdx;
		}
	};
}

} // namespace voxedit
