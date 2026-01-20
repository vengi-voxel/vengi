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
}

} // namespace voxedit
