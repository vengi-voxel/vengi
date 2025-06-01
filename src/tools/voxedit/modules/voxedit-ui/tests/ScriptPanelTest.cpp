/**
 * @file
 */

#include "../ScriptPanel.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void ScriptPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "create and save")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("New");
		ctx->Yield();
		IM_CHECK(saveFile(ctx, "test.lua"));
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemClick("Edit script");

		IM_CHECK(focusWindow(ctx, TITLE_SCRIPT_EDITOR));
		ctx->MouseMove("##MenuBar/File");
		ctx->MouseClick();
		ctx->MouseMove("##MenuBar/File");
		ctx->MouseClick();
		ctx->MenuClick("//$FOCUSED/Save as");
		IM_CHECK(cancelSaveFile(ctx));
	};
}

} // namespace voxedit
