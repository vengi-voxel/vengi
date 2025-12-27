/**
 * @file
 */

 #include "../NodePropertiesPanel.h"
 #include "voxedit-util/SceneManager.h"
 #include "TestUtil.h"

 namespace voxedit {

 void NodePropertiesPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	 IM_REGISTER_TEST(engine, testCategory(), "properties")->TestFunc = [=](ImGuiTestContext *ctx) {
		 IM_CHECK(activateViewportSceneMode(ctx, _app));
		 IM_CHECK(focusWindow(ctx, id));
		 ctx->ItemInputValue("##nodeproperties/##newpropertykey", "Key");
		 ctx->ItemInputValue("##nodeproperties/##newpropertyvalue", "Value");
		 ctx->ItemClick("##nodeproperties/###nodepropertyadd");
	 };
 }

 } // namespace voxedit
