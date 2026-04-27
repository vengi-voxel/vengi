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

	IM_REGISTER_TEST(engine, testCategory(), "grid size input")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		core::VarPtr gridSize = core::getVar(cfg::VoxEditGridsize);
		ctx->ItemInputValue("Grid size", 4);
		IM_CHECK_EQ(gridSize->intVal(), 4);
		ctx->ItemInputValue("Grid size", 1);
		IM_CHECK_EQ(gridSize->intVal(), 1);
	};
}

} // namespace voxedit
