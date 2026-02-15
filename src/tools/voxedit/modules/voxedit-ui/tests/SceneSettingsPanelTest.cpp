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

	IM_REGISTER_TEST(engine, testCategory(), "shading modes toggle")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, "shadingmodestest", voxel::Region(0, 31)));
		IM_CHECK(focusWindow(ctx, id));

		// switch to unlit
		ctx->ComboClick("Shading Mode/Unlit (Pure Colors)");
		ctx->Yield();
		IM_CHECK_EQ(_shadingMode->intVal(), (int)ShadingMode::Unlit);
		IM_CHECK_EQ(_rendershadow->boolVal(), false);

		// switch to lit
		ctx->ComboClick("Shading Mode/Lit (No Shadows)");
		ctx->Yield();
		IM_CHECK_EQ(_shadingMode->intVal(), (int)ShadingMode::Lit);
		IM_CHECK_EQ(_rendershadow->boolVal(), false);

		// switch to shadows
		ctx->ComboClick("Shading Mode/Shadows");
		ctx->Yield();
		IM_CHECK_EQ(_shadingMode->intVal(), (int)ShadingMode::Shadows);
		IM_CHECK_EQ(_rendershadow->boolVal(), true);
	};

	IM_REGISTER_TEST(engine, testCategory(), "sun presets")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, "sunpresetstest", voxel::Region(0, 31)));
		IM_CHECK(focusWindow(ctx, id));

		// enable shadows mode for sun presets to work
		ctx->ComboClick("Shading Mode/Shadows");
		ctx->Yield();

		ctx->ItemClick("sunangle/Preset: Noon");
		ctx->Yield();

		ctx->ItemClick("sunangle/Preset: Evening");
		ctx->Yield();

		ctx->ItemClick("sunangle/Preset: Morning");
		ctx->Yield();
	};
}

} // namespace voxedit
