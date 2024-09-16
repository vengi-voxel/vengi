/**
 * @file
 */

#include "../ToolsPanel.h"
#include "core/StringUtil.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void ToolsPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "toolbar")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		bool editMode = ctx->ItemInfo("edittools/###button0", ImGuiTestOpFlags_NoError).ID != 0;
		bool sceneMode = false;
		if (!editMode)
			sceneMode = ctx->ItemInfo("scenetools/###button0", ImGuiTestOpFlags_NoError).ID != 0;
		IM_CHECK(editMode || sceneMode);
		int buttonIdx = 0;
		for (;;) {
			core::String btnid = core::string::format("%s/###button%i", editMode ? "edittools" : "scenetools", buttonIdx);
			if (ctx->ItemInfo(btnid.c_str(), ImGuiTestOpFlags_NoError).ID == 0) {
				break;
			}
			ctx->LogInfo("Found button %i", buttonIdx);
			ctx->ItemClick(btnid.c_str());
			++buttonIdx;
		}
	};
}

} // namespace voxedit
