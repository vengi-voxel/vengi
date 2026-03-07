/**
 * @file
 */

#include "../OptionsPanel.h"
#include "TestUtil.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "voxel/SurfaceExtractor.h"

namespace voxedit {

void OptionsPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "toggle visibility")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		ctx->Yield();
		IM_CHECK(isVisible());
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();
		setVisible(false);
		ctx->Yield();
		IM_CHECK(!isVisible());
		setVisible(true);
		ctx->Yield();
		IM_CHECK(isVisible());
	};

	IM_REGISTER_TEST(engine, testCategory(), "select categories")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "cycle meshers")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(newTemplateScene(ctx, "##templates/##Knight"));

		setVisible(true);
		_selectedCategory = OptionCategory::Editor;
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();

		// Navigate to the content child window where the combo lives
		ImGuiTestItemInfo info = ctx->WindowInfo("##optionscontent");
		IM_CHECK(info.Window != nullptr);
		ctx->SetRef(info.Window);

		const core::VarPtr &meshMode = core::getVar(cfg::VoxelMeshMode);

		ctx->ComboClick("Mesh mode/Cubes");
		ctx->Yield();
		IM_CHECK_EQ(meshMode->intVal(), (int)voxel::SurfaceExtractionType::Cubic);

		ctx->ComboClick("Mesh mode/Marching cubes");
		ctx->Yield();
		IM_CHECK_EQ(meshMode->intVal(), (int)voxel::SurfaceExtractionType::MarchingCubes);

		ctx->ComboClick("Mesh mode/Binary");
		ctx->Yield();
		IM_CHECK_EQ(meshMode->intVal(), (int)voxel::SurfaceExtractionType::Binary);
	};
}

} // namespace voxedit
