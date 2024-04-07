/**
 * @file
 */

#include "../TreePanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void TreePanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	ImGuiTest *test = IM_REGISTER_TEST(engine, testCategory(), testName());
	test->TestFunc = [=](ImGuiTestContext *ctx) {
		ctx->SetRef(title);
	};
}

} // namespace voxedit
