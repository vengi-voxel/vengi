/**
 * @file
 */

#include "../ScriptPanel.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void ScriptPanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	IM_REGISTER_TEST(engine, testCategory(), "create and save")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, title));
		ctx->ItemClick("New");
		ctx->Yield();

		IM_CHECK(focusWindow(ctx, TITLE_SCRIPT_EDITOR));
		ctx->MouseMove("##menubar/###File");
		ctx->MouseClick();
		ctx->MenuClick("//$FOCUSED/###Apply and execute");
		ctx->MouseMove("##menubar/###File");
		ctx->MouseClick();
		ctx->MenuClick("//$FOCUSED/###Save as");
		IM_CHECK(saveFile(ctx, "test.lua"));
	};
}

} // namespace voxedit
