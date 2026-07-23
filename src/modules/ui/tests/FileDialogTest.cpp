/**
 * @file
 */

#include "../FileDialog.h"
#include "core/ConfigVar.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/Filesystem.h"
#include "ui/IMGUIApp.h"

namespace ui {

void FileDialog::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "bookmarks")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, "###app"));
		ctx->MenuClick("File/Load");

		video::OpenFileMode type = video::OpenFileMode::Open;
		IM_CHECK(focusWindow(ctx, popupTitle(type)));

		ctx->ItemClick("###addbookmark");

		ctx->SetRef(ctx->WindowInfo("bookmarks_child").ID);
		ctx->ItemClick("Bookmarks");

		ctx->MouseMove("Bookmarks/###0");
		ctx->MouseClick(ImGuiMouseButton_Right);
		ctx->MenuClick("//$FOCUSED/###Remove bookmark");

		IM_CHECK(focusWindow(ctx, popupTitle(type)));
		ctx->ItemClick("###Cancel");
	};

	IM_REGISTER_TEST(engine, testCategory(), "jump to entity on key")->TestFunc = [=](ImGuiTestContext *ctx) {
		const core::String testDir = app()->filesystem()->homeWritePath("filedialog_jump_test");
		const core::String dirA = core::string::path(testDir, "aaa_jumptest");
		const core::String dirZ = core::string::path(testDir, "zzz_jumptest");
		IM_CHECK(io::Filesystem::sysCreateDir(testDir));
		IM_CHECK(io::Filesystem::sysCreateDir(dirA));
		IM_CHECK(io::Filesystem::sysCreateDir(dirZ));
		const core::String absTestDir = app()->filesystem()->sysAbsolutePath(testDir);
		IM_CHECK(!absTestDir.empty());

		// Open the dialog already in the prepared directory.
		core::getVar(cfg::UILastDirectory)->setVal(absTestDir);

		IM_CHECK(focusWindow(ctx, "###app"));
		ctx->MenuClick("File/Load");

		video::OpenFileMode type = video::OpenFileMode::Open;
		IM_CHECK(focusWindow(ctx, popupTitle(type)));
		ctx->Yield();

		ImGuiTestItemInfo filesInfo = ctx->WindowInfo("files");
		IM_CHECK(filesInfo.Window != nullptr);
		ctx->SetRef(filesInfo.ID);

		// Ensure a non-z entry is selected first.
		ctx->ItemClick("aaa_jumptest");
		ctx->Yield();

		// Hover the file list so PlatformImeData.WantTextInput is set (same as real typing).
		ctx->MouseMoveToPos(filesInfo.Window->Rect().GetCenter());
		ctx->Yield();
		// WantTextInput is updated at EndFrame from PlatformImeData.
		ctx->Yield();
		IM_CHECK(ImGui::GetIO().WantTextInput);

		// Typing must still jump even though WantTextInput is true (entities panel IME request).
		onTextInput(nullptr, "z");
		ctx->Yield();

		IM_CHECK(focusWindow(ctx, popupTitle(type)));
		ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_C);
		ctx->Yield();
		const char *clipboard = ImGui::GetClipboardText();
		IM_CHECK(clipboard != nullptr);
		IM_CHECK(core::string::contains(clipboard, "zzz_jumptest"));

		ctx->ItemClick("###Cancel");

		io::Filesystem::sysRemoveDir(dirA);
		io::Filesystem::sysRemoveDir(dirZ);
		io::Filesystem::sysRemoveDir(testDir);
	};
}

} // namespace ui
