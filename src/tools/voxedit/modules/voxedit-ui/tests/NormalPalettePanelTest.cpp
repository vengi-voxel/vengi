/**
 * @file
 */

#include "../NormalPalettePanel.h"
#include "../ViewMode.h"
#include "TestUtil.h"
#include "command/CommandHandler.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void NormalPalettePanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "check existance")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(changeViewMode(ctx, ViewMode::RedAlert2));
		ImGuiWindow *window = ImGui::FindWindowByName(id);
		IM_CHECK(window != nullptr);
		IM_CHECK(window->Active);
	};

	IM_REGISTER_TEST(engine, testCategory(), "no existance")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(changeViewMode(ctx, ViewMode::Default));
		ImGuiWindow *window = ImGui::FindWindowByName(id);
		IM_CHECK(window != nullptr);
		IM_CHECK(!window->Active);
	};

	IM_REGISTER_TEST(engine, testCategory(), "switch built-in")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(changeViewMode(ctx, ViewMode::RedAlert2));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Tiberian Sun");
		ctx->MenuClick("File/Red Alert 2");
		ctx->MenuClick("File/Slab6");
	};

	// auto normals and remove all normals afterwards
	IM_REGISTER_TEST(engine, testCategory(), "auto normals")->TestFunc = [=](ImGuiTestContext *ctx) {
		// load a template model so there are voxels to calculate normals for
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(newTemplateScene(ctx, "##templates/##River"));
		IM_CHECK(changeViewMode(ctx, ViewMode::RedAlert2));
		IM_CHECK(focusWindow(ctx, id));

		// remove all normals first
		ctx->MenuClick("File/Remove all normals");
		ctx->Yield();

		// calculate normals with default (Flat) mode
		ctx->MenuClick("File/Auto normals");
		ctx->Yield();
		ctx->ItemClick("//$FOCUSED/Calculate normals");
		ctx->Yield();
	};

	// export normal palette
	IM_REGISTER_TEST(engine, testCategory(), "export normal palette")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(changeViewMode(ctx, ViewMode::RedAlert2));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Export");
		ctx->Yield();
		IM_CHECK(saveFile(ctx, "normalpalette-export.png"));
	};

	// change longitude and latitude
	IM_REGISTER_TEST(engine, testCategory(), "longitude latitude")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(_sceneMgr->newScene(true, ctx->Test->Name, voxel::Region(0, 31)));
		IM_CHECK(changeViewMode(ctx, ViewMode::RedAlert2));
		IM_CHECK(focusWindow(ctx, id));

		changeSlider(ctx, "Longitude", true);
		changeSlider(ctx, "Longitude", false);

		changeSlider(ctx, "Latitude", true);
		changeSlider(ctx, "Latitude", false);
	};
}

} // namespace voxedit
