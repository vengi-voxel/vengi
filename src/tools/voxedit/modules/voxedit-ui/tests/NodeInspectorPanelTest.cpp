/**
 * @file
 */

#include "../NodeInspectorPanel.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/SceneManager.h"
#include "TestUtil.h"

namespace voxedit {

void NodeInspectorPanel::registerUITests(ImGuiTestEngine *engine, const char *title) {
	IM_REGISTER_TEST(engine, testCategory(), "transform")->TestFunc = [=](ImGuiTestContext *ctx) {
		const int viewportId = viewportSceneMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);

		// by activating the edit mode viewport - we activate the brush panel
		const core::String id = Viewport::viewportId(viewportId);
		ctx->ItemClick(id.c_str());

		IM_CHECK(focusWindow(ctx, title));
		ctx->ItemInputValue("Tr/$$0", 1.0f);
		ctx->ItemInputValue("Tr/$$1", 2.0f);
		ctx->ItemInputValue("Tr/$$2", 3.0f);

		ctx->ItemInputValue("Rt/$$0", 45.0f);
		ctx->ItemInputValue("Rt/$$1", 90.0f);
		ctx->ItemInputValue("Rt/$$2", 45.0f);

		ctx->ItemInputValue("Sc/$$0", 2.0f);
		ctx->ItemInputValue("Sc/$$1", 2.0f);
		ctx->ItemInputValue("Sc/$$2", 2.0f);

		ctx->ItemInputValue("Pv/$$0", 1.0f);
		ctx->ItemInputValue("Pv/$$1", 1.0f);
		ctx->ItemInputValue("Pv/$$2", 1.0f);

		ctx->ItemClick("Reset all");
	};

	IM_REGISTER_TEST(engine, testCategory(), "properties")->TestFunc = [=](ImGuiTestContext *ctx) {
		const int viewportId = viewportSceneMode(ctx, _app);
		IM_CHECK_SILENT(viewportId != -1);

		// by activating the edit mode viewport - we activate the brush panel
		const core::String id = Viewport::viewportId(viewportId);
		ctx->ItemClick(id.c_str());

		IM_CHECK(focusWindow(ctx, title));
		ctx->ItemInputValue("##nodelist/##newpropertykey", "Key");
		ctx->ItemInputValue("##nodelist/##newpropertyvalue", "Value");
		ctx->ItemClick("##nodelist/###nodepropertyadd");
	};
}

} // namespace voxedit
