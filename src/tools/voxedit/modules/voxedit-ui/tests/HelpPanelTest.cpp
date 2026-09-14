/**
 * @file
 */

#include "../HelpPanel.h"
#include "core/Var.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void HelpPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	auto ensureVisible = [=](ImGuiTestContext *ctx) -> bool {
		core::getVar(cfg::VoxEditShowHelp)->setVal(true);
		ctx->Yield();
		return focusWindow(ctx, id);
	};

	// HelpContent is a child window; leftover scroll and clipped TextLink labels
	// make item lookup fail unless we pan from that child.
	auto refHelpContent = [=](ImGuiTestContext *ctx) -> bool {
		ImGuiTestItemInfo content = ctx->WindowInfo("HelpContent");
		if (content.Window == nullptr) {
			return false;
		}
		ctx->SetRef(content.Window);
		ctx->ScrollToTop("");
		return true;
	};

	IM_REGISTER_TEST(engine, testCategory(), "history navigation")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(ensureVisible(ctx));

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
		IM_CHECK(ensureVisible(ctx));
		setMarkdown("# Test Heading\n\nSome **bold** text and a [link](http://example.com).");
		ctx->Yield(3);
		IM_CHECK(!_markdown.empty());
	};

	IM_REGISTER_TEST(engine, testCategory(), "back and forward")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(ensureVisible(ctx));
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
		IM_CHECK(ensureVisible(ctx));
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

	IM_REGISTER_TEST(engine, testCategory(), "markdown table links")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(ensureVisible(ctx));
		init();
		ctx->Yield();
		setMarkdown("| Global | Description |\n| ------ | ----------- |\n| [g_cmd](../lua/cmd.md) | Command execution |\n");
		ctx->Yield(3);
		IM_CHECK(refHelpContent(ctx));
		IM_CHECK(ctx->ItemExists("**/g_cmd"));
		ctx->ItemClick("**/g_cmd");
		ctx->Yield();
		IM_CHECK(c()._filename == "cmd.md");
	};

	IM_REGISTER_TEST(engine, testCategory(), "markdown scripting api table links")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(ensureVisible(ctx));
		init();
		ctx->Yield();
		setMarkdownFile("../LUAScript.md");
		ctx->Yield(5);
		IM_CHECK(c()._filename == "LUAScript.md");
		IM_CHECK(_markdown.contains("[g_cmd]"));
		IM_CHECK(refHelpContent(ctx));
		IM_CHECK(ctx->ItemExists("**/g_cmd"));
		ctx->ItemClick("**/g_cmd");
		ctx->Yield();
		IM_CHECK(c()._filename == "cmd.md");
	};

	IM_REGISTER_TEST(engine, testCategory(), "markdown with images")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(ensureVisible(ctx));
		init();
		ctx->Yield();
		// Palette.md embeds a paletted PNG; regression for GL_UNPACK_ROW_LENGTH leftover from ImGui
		// texture updates causing heap-buffer-overflow on upload.
		setMarkdownFile("usage/Palette.md");
		ctx->Yield(5);
		IM_CHECK(c()._filename == "Palette.md");
		IM_CHECK(!_markdown.empty());
	};
}

} // namespace voxedit
