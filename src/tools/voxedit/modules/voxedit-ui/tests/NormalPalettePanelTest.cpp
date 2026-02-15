/**
 * @file
 */

#include "../NormalPalettePanel.h"
#include "../ViewMode.h"
#include "TestUtil.h"
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

	// TODO: auto normals and remove all normals afterwards
	// - load a model
	// - changeViewMode to redalert2
	// - call normpalette_removenormals
	// - call File->Auto normals->XXX actions with the different options
	// - call File->Remove all normals afterwards

	// TODO: File->Export normal palette to image and check the output

	// TODO: change longitude and latitude or modify the current normal index
}

} // namespace voxedit
