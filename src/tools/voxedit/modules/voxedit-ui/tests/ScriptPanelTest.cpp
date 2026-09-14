/**
 * @file
 */

#include "../ScriptPanel.h"
#include "TestUtil.h"
#include "core/String.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void ScriptPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	auto createAndSelectScript = [=](ImGuiTestContext *ctx, const char *filename) -> bool {
		if (!focusWindow(ctx, id)) {
			return false;
		}
		ctx->MenuClick("File/New");
		ctx->Yield();
		if (!saveFile(ctx, filename)) {
			return false;
		}
		if (!focusWindow(ctx, id)) {
			return false;
		}
		ctx->Yield(2);
		ctx->SetRef(id);
		ctx->ComboClick(core::String::format("##script/%s", filename).c_str());
		ctx->Yield(2);
		return focusWindow(ctx, id);
	};

	// Default view mode enables ve_showscript; ScopedViewMode restores Default after other modes.
	IM_REGISTER_TEST(engine, testCategory(), "create and save")->TestFunc = [=](ImGuiTestContext *ctx) {
		ScopedViewMode viewMode(ctx, ViewMode::Default);
		IM_CHECK(core::getVar(cfg::VoxEditShowScript)->boolVal());
		IM_CHECK(createAndSelectScript(ctx, "test.lua"));
		ctx->MenuClick("File/Edit script");

		IM_CHECK(focusWindow(ctx, TITLE_SCRIPT_EDITOR));
		ctx->MenuClick("File/Save as");
		IM_CHECK(cancelSaveFile(ctx));
	};

	IM_REGISTER_TEST(engine, testCategory(), "execute script")->TestFunc = [=](ImGuiTestContext *ctx) {
		ScopedViewMode viewMode(ctx, ViewMode::Default);
		IM_CHECK(core::getVar(cfg::VoxEditShowScript)->boolVal());
		IM_CHECK(createAndSelectScript(ctx, "test_run.lua"));
		ctx->SetRef(id);
		ctx->ItemClick("Run");
	};

	IM_REGISTER_TEST(engine, testCategory(), "reload scripts")->TestFunc = [=](ImGuiTestContext *ctx) {
		ScopedViewMode viewMode(ctx, ViewMode::Default);
		IM_CHECK(core::getVar(cfg::VoxEditShowScript)->boolVal());
		IM_CHECK(createAndSelectScript(ctx, "test_reload.lua"));

		ctx->MenuClick("File/Reload script");
		ctx->Yield();

		ctx->MenuClick("File/Reload all scripts");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "script editor edit menu")->TestFunc = [=](ImGuiTestContext *ctx) {
		ScopedViewMode viewMode(ctx, ViewMode::Default);
		IM_CHECK(core::getVar(cfg::VoxEditShowScript)->boolVal());
		IM_CHECK(createAndSelectScript(ctx, "test_editor.lua"));
		ctx->MenuClick("File/Edit script");
		ctx->Yield();

		IM_CHECK(focusWindow(ctx, TITLE_SCRIPT_EDITOR));

		ctx->MenuClick("Edit/Select all");
		ctx->Yield();

		ctx->MenuClick("Edit/Copy");
		ctx->Yield();

		ctx->MenuClick("Edit/Paste");
		ctx->Yield();

		ctx->MenuClick("File/Close");
		ctx->Yield();
	};
}

} // namespace voxedit
