/**
 * @file
 */

#include "../OptionsPanel.h"

namespace voxedit {

void OptionsPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "toggle visibility")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		ctx->Yield();
		IM_CHECK(isVisible());
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();
		setVisible(false);
		ctx->Yield();
		IM_CHECK(!isVisible());
		setVisible(true);
		ctx->Yield();
		IM_CHECK(isVisible());
	};

	IM_REGISTER_TEST(engine, testCategory(), "select categories")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();
	};
}

} // namespace voxedit
