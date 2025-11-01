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
}

} // namespace voxedit
