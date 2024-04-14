/**
 * @file
 */

#include "../TreePanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void TreePanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	IM_REGISTER_TEST(engine, testName(), "create tree")->TestFunc = [=](ImGuiTestContext *ctx) {
		ctx->SetRef(title);
		IM_CHECK(focusWindow(ctx, title));
	};
}

} // namespace voxedit
