/**
 * @file
 */

#include "../GameModePanel.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void GameModePanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "togglegamemode")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		const bool initialState = _gameModeEnabled;
		ctx->ItemClick("//###gamemodepanel/###Enable");
		IM_CHECK(_gameModeEnabled != initialState);
		ctx->ItemClick("//###gamemodepanel/###Enable");
		IM_CHECK(_gameModeEnabled == initialState);
	};

	IM_REGISTER_TEST(engine, testCategory(), "preset minecraft")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		// enable game mode first
		if (!_gameModeEnabled) {
			ctx->ItemClick("//###gamemodepanel/###Enable");
		}
		IM_CHECK(_gameModeEnabled);
		ctx->ItemClick("Minecraft");
		ctx->Yield();
		IM_CHECK_EQ(_bodyHeight->floatVal(), 1.8f);
		// disable game mode
		ctx->ItemClick("//###gamemodepanel/###Enable");
	};

	IM_REGISTER_TEST(engine, testCategory(), "preset ace of spades")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		// enable game mode first
		if (!_gameModeEnabled) {
			ctx->ItemClick("//###gamemodepanel/###Enable");
		}
		IM_CHECK(_gameModeEnabled);
		ctx->ItemClick("Ace Of Spades");
		ctx->Yield();
		IM_CHECK_EQ(_bodyHeight->floatVal(), 2.8f);
		// disable game mode
		ctx->ItemClick("//###gamemodepanel/###Enable");
	};
}

} // namespace voxedit
