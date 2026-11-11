/**
 * @file
 */

#include "../CommandPalette.h"
#include "command/CommandHandler.h"
#include "ui/IMGUIApp.h"
#include "ui/Panel.h"

namespace ui {

void CommandPalette::registerUITests(ImGuiTestEngine *engine, const char *id) {
	auto closePalette = [](ImGuiTestContext *ctx) {
		if (ImGui::GetTopMostPopupModal() != nullptr) {
			ctx->KeyPress(ImGuiKey_Escape);
			ctx->Yield(2);
		}
	};

	auto focusPalette = [](ImGuiTestContext *ctx) -> bool {
		Panel *panel = imguiApp()->findPanel("filedialog");
		if (panel == nullptr) {
			return false;
		}
		return panel->focusWindow(ctx, POPUP_TITLE_COMMAND_PALETTE);
	};

	IM_REGISTER_TEST(engine, testCategory(), "open and cancel")->TestFunc = [=](ImGuiTestContext *ctx) {
		Panel *panel = imguiApp()->findPanel("filedialog");
		IM_CHECK(panel != nullptr);
		IM_CHECK(panel->focusWindow(ctx, "###app"));
		command::executeCommands("ui_commandpalette");
		ctx->Yield(2);
		IM_CHECK(Panel::isPopupOpen(POPUP_TITLE_COMMAND_PALETTE));
		IM_CHECK(focusPalette(ctx));
		ctx->ItemClick("###Cancel");
		ctx->Yield(2);
		IM_CHECK(!Panel::isPopupOpen(POPUP_TITLE_COMMAND_PALETTE));
	};
	IM_REGISTER_TEST(engine, testCategory(), "open and cancel")->TeardownFunc = closePalette;

	IM_REGISTER_TEST(engine, testCategory(), "filter and run")->TestFunc = [=](ImGuiTestContext *ctx) {
		Panel *panel = imguiApp()->findPanel("filedialog");
		IM_CHECK(panel != nullptr);
		IM_CHECK(panel->focusWindow(ctx, "###app"));
		show();
		ctx->Yield(2);
		IM_CHECK(Panel::isPopupOpen(POPUP_TITLE_COMMAND_PALETTE));
		IM_CHECK(focusPalette(ctx));

		ctx->ItemClick("##commandpalettefilter");
		ctx->KeyCharsReplace("con_clear");
		ctx->Yield();
		ctx->KeyPress(ImGuiKey_Enter);
		ctx->Yield(2);
		IM_CHECK(!Panel::isPopupOpen(POPUP_TITLE_COMMAND_PALETTE));
	};
	IM_REGISTER_TEST(engine, testCategory(), "filter and run")->TeardownFunc = closePalette;

	IM_REGISTER_TEST(engine, testCategory(), "params visible after selection")->TestFunc = [=](ImGuiTestContext *ctx) {
		Panel *panel = imguiApp()->findPanel("filedialog");
		IM_CHECK(panel != nullptr);
		IM_CHECK(panel->focusWindow(ctx, "###app"));
		show();
		ctx->Yield(2);
		IM_CHECK(Panel::isPopupOpen(POPUP_TITLE_COMMAND_PALETTE));
		IM_CHECK(focusPalette(ctx));

		ctx->ItemClick("##commandpalettefilter");
		ctx->KeyCharsReplace("alias");
		ctx->Yield();
		IM_CHECK(ctx->ItemExists("name"));
		ctx->ItemClick("###Cancel");
		ctx->Yield(2);
		IM_CHECK(!Panel::isPopupOpen(POPUP_TITLE_COMMAND_PALETTE));
	};
	IM_REGISTER_TEST(engine, testCategory(), "params visible after selection")->TeardownFunc = closePalette;
}

} // namespace ui
