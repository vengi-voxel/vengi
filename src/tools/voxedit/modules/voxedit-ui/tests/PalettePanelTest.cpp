/**
 * @file
 */

#include "../PalettePanel.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void PalettePanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	IM_REGISTER_TEST(engine, testCategory(), "switch built-in")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, title));
		for (int i = lengthof(palette::Palette::builtIn) - 1; i >= 0; --i) {
			ctx->SetRef(title);
			ctx->MenuClick("###File/###Switch");
			ctx->SetRef(POPUP_TITLE_LOAD_PALETTE);
			ctx->ItemClick("##type");
			core::String name = core::string::format("//$FOCUSED/%s", palette::Palette::builtIn[i]);
			ctx->ItemClick(name.c_str());
			ctx->ItemClick("###OK");
		}
	};
}

} // namespace voxedit
