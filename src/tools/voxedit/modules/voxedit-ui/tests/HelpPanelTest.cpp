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

	IM_REGISTER_TEST(engine, testCategory(), "markdown rendering")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		setMarkdown("# Test Heading\n\nSome **bold** text and a [link](http://example.com).");
		ctx->Yield(3);
		IM_CHECK(!_markdown.empty());
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

	IM_REGISTER_TEST(engine, testCategory(), "internal link navigation")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		init();
		ctx->Yield();

		// navigate to Features.md via setMarkdownFile (simulates clicking an internal link)
		const int posBefore = _historyPosition;
		setMarkdownFile("Features.md");
		ctx->Yield(3);
		IM_CHECK(_historyPosition > posBefore);
		IM_CHECK(c()._filename == "Features.md");
		IM_CHECK(canGoBack());

		// navigate to another page
		setMarkdownFile("Index.md");
		ctx->Yield(3);
		IM_CHECK(c()._filename == "Index.md");

		// go back via the Back button
		ctx->ItemClick("Back###Back");
		ctx->Yield();
		IM_CHECK(c()._filename == "Features.md");
		IM_CHECK(canGoForward());
	};
}

} // namespace voxedit
