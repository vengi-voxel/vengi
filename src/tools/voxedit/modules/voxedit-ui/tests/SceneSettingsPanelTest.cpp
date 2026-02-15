/**
 * @file
 */

#include "../SceneSettingsPanel.h"
#include "TestUtil.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void SceneSettingsPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "shading")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(newTemplateScene(ctx, "##templates/##River"));
		IM_CHECK(focusWindow(ctx, id));

		ctx->ComboClick("Shading Mode/Unlit (Pure Colors)");
		ctx->ComboClick("Shading Mode/Lit (No Shadows)");
		ctx->ComboClick("Shading Mode/Shadows");

		changeSlider(ctx, "sunangle/Azimuth", true);
		changeSlider(ctx, "sunangle/Azimuth", false);
	};
}

} // namespace voxedit
