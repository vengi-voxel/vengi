/**
 * @file
 */

#include "../HelpPanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void HelpPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	// TODO: Add tests for history navigation functionality
#if 0
	IM_REGISTER_TEST(engine, testCategory(), "history navigation")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));

		// Test forward/backward navigation
		IM_CHECK(!canGoBack());
		IM_CHECK(!canGoForward());

		setMarkdownFile("Installation.md");
		IM_CHECK(canGoBack());
		IM_CHECK(!canGoForward());

		goBack();
		IM_CHECK(!canGoBack());
		IM_CHECK(canGoForward());

		goForward();
		IM_CHECK(canGoBack());
		IM_CHECK(!canGoForward());
	};
#endif
}

} // namespace voxedit
