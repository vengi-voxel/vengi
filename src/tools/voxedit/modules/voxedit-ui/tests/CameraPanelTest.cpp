/**
 * @file
 */

#include "../CameraPanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void CameraPanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	ImGuiTest *test = IM_REGISTER_TEST(engine, testCategory(), testName());
	test->TestFunc = [=](ImGuiTestContext *ctx) {
		ctx->SetRef(title);
		ctx->ItemInputValue("Position/$$0", 90.0f);
		ctx->ItemInputValue("FOV", 90.0f);
	};
}

} // namespace voxedit
