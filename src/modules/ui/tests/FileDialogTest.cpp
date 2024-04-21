/**
 * @file
 */

#include "../FileDialog.h"
#include "ui/IMGUIApp.h"

namespace ui {

void FileDialog::registerUITests(ImGuiTestEngine *engine, const char *title) {
	IM_REGISTER_TEST(engine, testCategory(), "bookmarks")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, "###app"));
		ctx->MenuClick("###File/###Load");

		video::OpenFileMode type = video::OpenFileMode::Open;
		IM_CHECK(focusWindow(ctx, popupTitle(type)));

		ctx->ItemClick("###addbookmark");

		ctx->SetRef(ctx->WindowInfo("bookmarks_child")->ID);
		ctx->ItemClick("Bookmarks");

		ctx->MouseMove("Bookmarks/###0");
		ctx->MouseClick(ImGuiMouseButton_Right);
		ctx->MenuClick("//$FOCUSED/###Remove bookmark");

		IM_CHECK(focusWindow(ctx, popupTitle(type)));
		ctx->ItemClick("Cancel");
	};
}

} // namespace ui
