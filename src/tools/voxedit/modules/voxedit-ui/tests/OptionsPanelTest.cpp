/**
 * @file
 */

#include "../OptionsPanel.h"
#include "TestUtil.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "voxedit-util/Config.h"
#include "voxel/SurfaceExtractor.h"

namespace voxedit {

void OptionsPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "toggle visibility")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		_filter = "";
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
		_filter = "";
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

	IM_REGISTER_TEST(engine, testCategory(), "display toggles")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		_filter = "";
		_selectedCategory = OptionCategory::Display;
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();

		ImGuiTestItemInfo info = ctx->WindowInfo("##optionscontent");
		IM_CHECK(info.Window != nullptr);
		ctx->SetRef(info.Window);

		const core::VarPtr &showGrid = core::getVar(cfg::VoxEditShowgrid);
		const core::VarPtr &showAxis = core::getVar(cfg::VoxEditShowaxis);
		const core::VarPtr &showAabb = core::getVar(cfg::VoxEditShowaabb);
		const core::VarPtr &showBones = core::getVar(cfg::VoxEditShowBones);

		// Toggle grid off and on
		const bool gridBefore = showGrid->boolVal();
		ctx->ItemClick("Grid");
		ctx->Yield();
		IM_CHECK_EQ(showGrid->boolVal(), !gridBefore);
		ctx->ItemClick("Grid");
		ctx->Yield();
		IM_CHECK_EQ(showGrid->boolVal(), gridBefore);

		// Toggle axis
		const bool axisBefore = showAxis->boolVal();
		ctx->ItemClick("Show gizmo");
		ctx->Yield();
		IM_CHECK_EQ(showAxis->boolVal(), !axisBefore);
		ctx->ItemClick("Show gizmo");
		ctx->Yield();
		IM_CHECK_EQ(showAxis->boolVal(), axisBefore);

		// Toggle bounding box
		const bool aabbBefore = showAabb->boolVal();
		ctx->ItemClick("Bounding box");
		ctx->Yield();
		IM_CHECK_EQ(showAabb->boolVal(), !aabbBefore);
		ctx->ItemClick("Bounding box");
		ctx->Yield();
		IM_CHECK_EQ(showAabb->boolVal(), aabbBefore);

		// Toggle bones
		const bool bonesBefore = showBones->boolVal();
		ctx->ItemClick("Bones");
		ctx->Yield();
		IM_CHECK_EQ(showBones->boolVal(), !bonesBefore);
		ctx->ItemClick("Bones");
		ctx->Yield();
		IM_CHECK_EQ(showBones->boolVal(), bonesBefore);
	};

	IM_REGISTER_TEST(engine, testCategory(), "rendering toggles")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(newTemplateScene(ctx, "##templates/##Knight"));

		setVisible(true);
		_filter = "";
		_selectedCategory = OptionCategory::Rendering;
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();

		ImGuiTestItemInfo info = ctx->WindowInfo("##optionscontent");
		IM_CHECK(info.Window != nullptr);
		ctx->SetRef(info.Window);

		// Set mesh mode to Cubic first so rendering checkboxes are not disabled
		core::getVar(cfg::VoxelMeshMode)->setVal((int)voxel::SurfaceExtractionType::Cubic);
		ctx->Yield();

		const core::VarPtr &outline = core::getVar(cfg::RenderOutline);
		const core::VarPtr &checkerBoard = core::getVar(cfg::RenderCheckerBoard);
		const core::VarPtr &bloom = core::getVar(cfg::ClientBloom);

		// Toggle outline
		const bool outlineBefore = outline->boolVal();
		ctx->ItemClick("Outlines");
		ctx->Yield();
		IM_CHECK_EQ(outline->boolVal(), !outlineBefore);
		ctx->ItemClick("Outlines");
		ctx->Yield();
		IM_CHECK_EQ(outline->boolVal(), outlineBefore);

		// Toggle checkerboard
		const bool checkerBefore = checkerBoard->boolVal();
		ctx->ItemClick("Checkerboard");
		ctx->Yield();
		IM_CHECK_EQ(checkerBoard->boolVal(), !checkerBefore);
		ctx->ItemClick("Checkerboard");
		ctx->Yield();
		IM_CHECK_EQ(checkerBoard->boolVal(), checkerBefore);

		// Toggle bloom
		const bool bloomBefore = bloom->boolVal();
		ctx->ItemClick("Bloom");
		ctx->Yield();
		IM_CHECK_EQ(bloom->boolVal(), !bloomBefore);
		ctx->ItemClick("Bloom");
		ctx->Yield();
		IM_CHECK_EQ(bloom->boolVal(), bloomBefore);
	};

	IM_REGISTER_TEST(engine, testCategory(), "search filter")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		_filter = "";
		_selectedCategory = OptionCategory::AllVariables;
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));

		ctx->ItemInputValue("Search", "gridsize");
		ctx->Yield(3);
		IM_CHECK(!_filter.empty());

		// clear the filter
		ctx->ItemInputValue("Search", "");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "category switching")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		_filter = "";
		IM_CHECK(focusWindow(ctx, id));

		_selectedCategory = OptionCategory::UserInterface;
		ctx->Yield();
		_selectedCategory = OptionCategory::Editor;
		ctx->Yield();
		_selectedCategory = OptionCategory::Rendering;
		ctx->Yield();
		_selectedCategory = OptionCategory::Renderer;
		ctx->Yield();
		_selectedCategory = OptionCategory::UserInterface;
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "editor settings")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		_filter = "";
		_selectedCategory = OptionCategory::Editor;
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();

		ImGuiTestItemInfo info = ctx->WindowInfo("##optionscontent");
		IM_CHECK(info.Window != nullptr);
		ctx->SetRef(info.Window);

		const core::VarPtr &animSpeed = core::getVar(cfg::VoxEditAnimationSpeed);
		ctx->ItemInputValue("Model animation speed", 200);
		ctx->Yield();
		IM_CHECK_EQ(animSpeed->intVal(), 200);
		ctx->ItemInputValue("Model animation speed", 100);
	};

	IM_REGISTER_TEST(engine, testCategory(), "renderer settings")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		_filter = "";
		_selectedCategory = OptionCategory::Renderer;
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();

		ImGuiTestItemInfo info = ctx->WindowInfo("##optionscontent");
		IM_CHECK(info.Window != nullptr);
		ctx->SetRef(info.Window);

		const core::VarPtr &vsync = core::getVar(cfg::ClientVSync);
		const bool vsyncBefore = vsync->boolVal();
		ctx->ItemClick("VSync");
		ctx->Yield();
		IM_CHECK(vsync->boolVal() != vsyncBefore);
		ctx->ItemClick("VSync");
		ctx->Yield();
		IM_CHECK(vsync->boolVal() == vsyncBefore);
	};

	IM_REGISTER_TEST(engine, testCategory(), "format options")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		_filter = "";
		_selectedCategory = OptionCategory::MeshExport;
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();

		ImGuiTestItemInfo info = ctx->WindowInfo("##optionscontent");
		IM_CHECK(info.Window != nullptr);
		ctx->SetRef(info.Window);

		const core::VarPtr &mergeQuads = core::getVar(cfg::VoxformatMergequads);
		const bool before = mergeQuads->boolVal();
		ctx->ItemClick("Merge quads");
		ctx->Yield();
		IM_CHECK(mergeQuads->boolVal() != before);
		ctx->ItemClick("Merge quads");
		ctx->Yield();
		IM_CHECK(mergeQuads->boolVal() == before);
	};

	IM_REGISTER_TEST(engine, testCategory(), "ui settings")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		_filter = "";
		_selectedCategory = OptionCategory::UserInterface;
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();

		ImGuiTestItemInfo info = ctx->WindowInfo("##optionscontent");
		IM_CHECK(info.Window != nullptr);
		ctx->SetRef(info.Window);

		const core::VarPtr &fontSize = core::getVar(cfg::UIFontSize);
		const int fontBefore = fontSize->intVal();
		ctx->ItemInputValue("Font size", fontBefore + 2);
		ctx->Yield();
		IM_CHECK_EQ(fontSize->intVal(), fontBefore + 2);
		// restore
		fontSize->setVal(fontBefore);
	};

	IM_REGISTER_TEST(engine, testCategory(), "metrics settings")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		_filter = "";
		_selectedCategory = OptionCategory::Metrics;
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();

		ImGuiTestItemInfo info = ctx->WindowInfo("##optionscontent");
		IM_CHECK(info.Window != nullptr);
	};

	IM_REGISTER_TEST(engine, testCategory(), "advanced variables")->TestFunc = [=](ImGuiTestContext *ctx) {
		setVisible(true);
		_filter = "";
		_selectedCategory = OptionCategory::AllVariables;
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield();

		ImGuiTestItemInfo info = ctx->WindowInfo("##optionscontent");
		IM_CHECK(info.Window != nullptr);
		ctx->SetRef(info.Window);

		// the cvars table should exist
		const ImGuiTestItemInfo table = ctx->ItemInfo("##cvars", ImGuiTestOpFlags_NoError);
		IM_CHECK(table.ID != 0);
	};
}

} // namespace voxedit
