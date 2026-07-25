/**
 * @file
 */

#include "../StatusBar.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void StatusBar::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "toggle settings")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		const struct {
			const char *cvarName;
			const char *label;
		} checkboxes[]{{cfg::VoxEditGrayInactive, "Grayscale"}, {cfg::VoxEditHideInactive, "Only active"}};
		for (int i = 0; i < lengthof(checkboxes); ++i) {
			core::VarPtr var = core::getVar(checkboxes[i].cvarName);
			const bool before = var->boolVal();
			ctx->ItemClick(checkboxes[i].label);
			IM_CHECK(before != var->boolVal());
			ctx->ItemClick(checkboxes[i].label);
			IM_CHECK(before == var->boolVal());
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "no grid size input")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		// Grid size moved to the Tools panel Cursor section
		IM_CHECK(!ctx->ItemExists("Grid size"));
		IM_CHECK(!ctx->ItemExists("**/Grid size"));
	};
}

} // namespace voxedit
