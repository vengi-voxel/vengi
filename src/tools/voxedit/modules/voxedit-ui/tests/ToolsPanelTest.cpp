/**
 * @file
 */

#include "../ToolsPanel.h"
#include "core/StringUtil.h"
#include "TestUtil.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void ToolsPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "scenetools")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));
		int buttonIdx = 0;
		for (;;) {
			const core::String &btnid = core::String::format("scenetools/###button%i", buttonIdx);
			if (ctx->ItemInfo(btnid.c_str(), ImGuiTestOpFlags_NoError).ID == 0) {
				break;
			}
			ctx->LogInfo("Found button %i", buttonIdx);
			ctx->ItemClick(btnid.c_str());
			++buttonIdx;
		}
		IM_CHECK(buttonIdx > 0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "edittools")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportEditMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));
		int buttonIdx = 0;
		for (;;) {
			const core::String &btnid = core::String::format("edittools/###button%i", buttonIdx);
			if (ctx->ItemInfo(btnid.c_str(), ImGuiTestOpFlags_NoError).ID == 0) {
				break;
			}
			ctx->LogInfo("Found button %i", buttonIdx);
			ctx->ItemClick(btnid.c_str());
			++buttonIdx;
		}
		IM_CHECK(buttonIdx > 0);
	};
}

} // namespace voxedit
