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

	IM_REGISTER_TEST(engine, testCategory(), "back and forward")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		// reset to home
		init();
		ctx->Yield();
		IM_CHECK(!canGoBack());

		// navigate to a page
		setMarkdownFile("Features.md");
		ctx->Yield();
		IM_CHECK(canGoBack());
		IM_CHECK(!canGoForward());

		// navigate to another page
		setMarkdownFile("Index.md");
		ctx->Yield();
		IM_CHECK(canGoBack());
		IM_CHECK(!canGoForward());

		// go back
		ctx->ItemClick("Back");
		ctx->Yield();
		IM_CHECK(canGoBack());
		IM_CHECK(canGoForward());

		// go forward
		ctx->ItemClick("Forward");
		ctx->Yield();
		IM_CHECK(canGoBack());
		IM_CHECK(!canGoForward());

		// go back to home
		ctx->ItemClick("Home");
		ctx->Yield();
		IM_CHECK(!canGoBack());
		IM_CHECK(!canGoForward());
	};
}

} // namespace voxedit
