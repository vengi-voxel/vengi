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
		ctx->MenuClick("File/New");
		ctx->Yield();
		IM_CHECK(saveFile(ctx, "test.lua"));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Edit script");

		IM_CHECK(focusWindow(ctx, TITLE_SCRIPT_EDITOR));
		ctx->MenuClick("File/Save as");
		IM_CHECK(cancelSaveFile(ctx));
	};

	IM_REGISTER_TEST(engine, testCategory(), "execute script")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/New");
		ctx->Yield();
		IM_CHECK(saveFile(ctx, "test_run.lua"));
		ctx->SetRef(id);
		ctx->ComboClick("##script/test_run.lua");
		ctx->ItemClick("Run");
	};
}

} // namespace voxedit
