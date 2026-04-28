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

	IM_REGISTER_TEST(engine, testCategory(), "reload scripts")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		// create a script first so reload has something to work with
		ctx->MenuClick("File/New");
		ctx->Yield();
		IM_CHECK(saveFile(ctx, "test_reload.lua"));
		ctx->SetRef(id);
		ctx->ComboClick("##script/test_reload.lua");
		ctx->Yield();

		// reload the current script
		ctx->MenuClick("File/Reload script");
		ctx->Yield();

		// reload all scripts
		ctx->MenuClick("File/Reload all scripts");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "script editor edit menu")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		// create a script and open the editor
		ctx->MenuClick("File/New");
		ctx->Yield();
		IM_CHECK(saveFile(ctx, "test_editor.lua"));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Edit script");
		ctx->Yield();

		IM_CHECK(focusWindow(ctx, TITLE_SCRIPT_EDITOR));

		// test Edit menu items
		ctx->MenuClick("Edit/Select all");
		ctx->Yield();

		ctx->MenuClick("Edit/Copy");
		ctx->Yield();

		ctx->MenuClick("Edit/Paste");
		ctx->Yield();

		// close the editor
		ctx->MenuClick("File/Close");
		ctx->Yield();
	};
}

} // namespace voxedit
