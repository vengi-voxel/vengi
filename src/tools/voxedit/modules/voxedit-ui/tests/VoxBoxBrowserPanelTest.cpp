/**
 * @file
 */

#include "../VoxBoxBrowserPanel.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void VoxBoxBrowserPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "login required")->TestFunc = [=](ImGuiTestContext *ctx) {
		_api.logout();
		_varApiKey->setVal("");
		_useApiKey = false;
		open();
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));

		IM_CHECK(ctx->ItemExists("##voxbox_user"));
		IM_CHECK(ctx->ItemExists("##voxbox_pass"));
		IM_CHECK(ctx->ItemExists("###Login"));
		IM_CHECK(ctx->ItemExists("Use API key"));
		IM_CHECK(!ctx->ItemExists("###Search"));
		IM_CHECK(!ctx->ItemExists("##voxboxfilter"));
	};

	IM_REGISTER_TEST(engine, testCategory(), "switch to api key")->TestFunc = [=](ImGuiTestContext *ctx) {
		_api.logout();
		_varApiKey->setVal("");
		_useApiKey = false;
		open();
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));

		ctx->ItemClick("Use API key");
		ctx->Yield();
		IM_CHECK(ctx->ItemExists("##voxbox_apikey"));
		IM_CHECK(ctx->ItemExists("###Connect"));
		IM_CHECK(!ctx->ItemExists("##voxbox_user"));
		IM_CHECK(!ctx->ItemExists("###Search"));

		ctx->ItemClick("Use password");
		ctx->Yield();
		IM_CHECK(ctx->ItemExists("##voxbox_user"));
		IM_CHECK(ctx->ItemExists("###Login"));
		IM_CHECK(!ctx->ItemExists("##voxbox_apikey"));
	};
}

} // namespace voxedit
