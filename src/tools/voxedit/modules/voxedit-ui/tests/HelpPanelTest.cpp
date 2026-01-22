/**
 * @file
 */

#include "../HelpPanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void HelpPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "history navigation")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));

		// Click Home if the button is available
		if (canGoBack() || canGoForward()) {
			ctx->ItemClick("Home");
		}
		ctx->Yield();

		IM_CHECK(!canGoBack());
		IM_CHECK(!canGoForward());

		setMarkdownFile("Features.md");
		IM_CHECK(canGoBack());
		IM_CHECK(!canGoForward());
	};
}

} // namespace voxedit
