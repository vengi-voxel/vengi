/**
 * @file
 */

#include "../PalettePanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void PalettePanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
#if 0
	IM_REGISTER_TEST(engine, testCategory(), "none")->TestFunc = [=](ImGuiTestContext *ctx) {
		ctx->SetRef(title);
		IM_CHECK(focusWindow(ctx, title));
	};
#endif
}

} // namespace voxedit
