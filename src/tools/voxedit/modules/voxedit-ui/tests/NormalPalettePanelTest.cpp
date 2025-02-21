/**
 * @file
 */

#include "../NormalPalettePanel.h"
#include "../ViewMode.h"
#include "util/VarUtil.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void NormalPalettePanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "none")->TestFunc = [=](ImGuiTestContext *ctx) {
		// Activate viewmode Command&Conquer to see the normal palette panel
		util::ScopedVarChange viewMode(cfg::VoxEditViewMode, (int)ViewMode::CommandAndConquer);
		ctx->Yield(); // allow the cvar change to propagate to the ui

		IM_CHECK(focusWindow(ctx, id));
	};
}

} // namespace voxedit
