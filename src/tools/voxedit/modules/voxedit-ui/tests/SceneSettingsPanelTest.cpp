/**
 * @file
 */

#include "../SceneSettingsPanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void SceneSettingsPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
#if 0
	IM_REGISTER_TEST(engine, testCategory(), "none")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
	};
#endif
}

} // namespace voxedit
