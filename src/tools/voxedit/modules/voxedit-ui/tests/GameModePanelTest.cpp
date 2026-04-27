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
		if (!_gameModeEnabled) {
			ctx->ItemClick("//###gamemodepanel/###Enable");
		}
		IM_CHECK(_gameModeEnabled);
		ctx->ItemClick("Ace Of Spades");
		ctx->Yield();
		IM_CHECK_EQ(_bodyHeight->floatVal(), 2.8f);
		ctx->ItemClick("//###gamemodepanel/###Enable");
	};

	IM_REGISTER_TEST(engine, testCategory(), "preset teardown")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		if (!_gameModeEnabled) {
			ctx->ItemClick("//###gamemodepanel/###Enable");
		}
		IM_CHECK(_gameModeEnabled);
		ctx->ItemClick("Teardown");
		ctx->Yield();
		IM_CHECK_EQ(_bodyHeight->floatVal(), 18.8f);
		ctx->ItemClick("//###gamemodepanel/###Enable");
	};

	IM_REGISTER_TEST(engine, testCategory(), "float inputs")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		if (!_gameModeEnabled) {
			ctx->ItemClick("//###gamemodepanel/###Enable");
		}
		IM_CHECK(_gameModeEnabled);

		ctx->ItemInputValue("Movement speed", 42.0f);
		IM_CHECK_EQ(_movementSpeed->floatVal(), 42.0f);

		ctx->ItemInputValue("Sprint multiplier", 5.0f);
		IM_CHECK_EQ(_sprintMultiplier->floatVal(), 5.0f);

		ctx->ItemInputValue("Jump velocity", 12.0f);
		IM_CHECK_EQ(_jumpVelocity->floatVal(), 12.0f);

		ctx->ItemClick("//###gamemodepanel/###Enable");
	};
}

} // namespace voxedit
