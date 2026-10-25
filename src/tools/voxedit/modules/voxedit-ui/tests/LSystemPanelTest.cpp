/**
 * @file
 */

#include "../LSystemPanel.h"
#include "TestUtil.h"
#include "core/Var.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

void LSystemPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "default rule")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/L-System", cfg::VoxEditShowLSystem));
		IM_CHECK(focusWindow(ctx, id));
		const voxel::RawVolume *volume = _sceneMgr->volume(_sceneMgr->activeNodeUUID());
		IM_CHECK(volume != nullptr);

		ctx->ItemInputValue("Iterations", 1);
		ctx->ItemClick("###Ok");
		ctx->Yield(1); // give a chance to execute at least one step of the lsystem generation
		_sceneMgr->lsystemAbort();
		IM_CHECK(voxelutil::countVoxels(*volume) > 0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "template selection")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/L-System", cfg::VoxEditShowLSystem));
		IM_CHECK(focusWindow(ctx, id));

		// verify the Templates combo exists and has entries
		if (!_templates.empty()) {
			const core::String &firstName = _templates[0].name;
			ctx->ComboClick(core::String::format("Templates/%s", firstName.c_str()).c_str());
			ctx->Yield();
			// after selecting a template, the axiom should be filled
			IM_CHECK(!_conf.axiom.empty());
			IM_CHECK_EQ(_conf.leafColor, voxelgenerator::lsystem::defaultLeafColor(_sceneMgr->activePalette()));
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "default leaf color")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/L-System", cfg::VoxEditShowLSystem));
		IM_CHECK(focusWindow(ctx, id));
		const int expected = voxelgenerator::lsystem::defaultLeafColor(_sceneMgr->activePalette());
		IM_CHECK(expected >= 0);
		IM_CHECK_EQ(_conf.leafColor, expected);
	};

	IM_REGISTER_TEST(engine, testCategory(), "branch color readonly")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/L-System", cfg::VoxEditShowLSystem));
		IM_CHECK(focusWindow(ctx, id));

		_sceneMgr->modifier().setCursorVoxel(voxel::createVoxel(voxel::VoxelType::Generic, 5));
		ctx->Yield();
		const ImGuiTestItemInfo item = ctx->ItemInfo("Branch color");
		IM_CHECK(item.ID != 0);
		IM_CHECK_EQ(_sceneMgr->modifier().cursorVoxel().getColor(), 5);

		ctx->ItemInputValue("Branch color", 7);
		IM_CHECK_EQ(_sceneMgr->modifier().cursorVoxel().getColor(), 5);
	};

	IM_REGISTER_TEST(engine, testCategory(), "parameter changes")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/L-System", cfg::VoxEditShowLSystem));
		IM_CHECK(focusWindow(ctx, id));

		ctx->ItemInputValue("Angle", 45.0f);
		IM_CHECK_EQ((int)glm::degrees(_conf.angle), 45);

		ctx->ItemInputValue("Length", 5.0f);
		IM_CHECK_EQ((int)_conf.length, 5);

		ctx->ItemInputValue("Length scale", 0.85f);
		IM_CHECK(_conf.lengthScale > 0.84f && _conf.lengthScale < 0.86f);
		ctx->ItemInputValue("Minimum length", 1.0f);
		IM_CHECK_EQ(_conf.minLength, 1.0f);
		ctx->ItemInputValue("Angle variation", 5.0f);
		IM_CHECK_EQ((int)glm::degrees(_conf.angleVariation), 5);
		ctx->ItemInputValue("Seed", 42);
		IM_CHECK_EQ(_conf.seed, 42);
		ctx->ItemInputValue("Leaf color", 2);
		IM_CHECK_EQ(_conf.leafColor, 2);
		ctx->ItemInputValue("Iterations", 3);
		IM_CHECK_EQ(_conf.iterations, 3);
	};

	IM_REGISTER_TEST(engine, testCategory(), "leaf color palette drop")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/Palette", cfg::VoxEditShowPalette));
		IM_CHECK(ensurePanelVisible(ctx, "View/L-System", cfg::VoxEditShowLSystem));
		IM_CHECK(focusWindow(ctx, id));

		ctx->ItemInputValue("Leaf color", -1);
		IM_CHECK_EQ(_conf.leafColor, -1);

		ImGuiWindow *palWindow = ImGui::FindWindowByName(TITLE_PALETTE);
		IM_CHECK(palWindow != nullptr);
		const float cellSize = ImGui::GetFrameHeight() * core::getVar(cfg::VoxEditPaletteColorSize)->floatVal();
		const ImVec2 palPos(palWindow->ContentRegionRect.Min.x + cellSize * 2.5f,
							palWindow->ContentRegionRect.Min.y + cellSize * 0.5f);
		const ImGuiTestItemInfo item = ctx->ItemInfo("Leaf color");
		IM_CHECK(item.ID != 0);

		dragFromTo(ctx, palPos, item.RectFull.GetCenter());
		const int expected = _sceneMgr->activePalette().view().uiIndex(2);
		IM_CHECK_EQ(_conf.leafColor, expected);
	};

	IM_REGISTER_TEST(engine, testCategory(), "add delete rule")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/L-System", cfg::VoxEditShowLSystem));
		IM_CHECK(focusWindow(ctx, id));

		const int rulesBefore = (int)_conf.rules.size();
		ctx->ItemClick("Add Rule###Add Rule");
		ctx->Yield();
		IM_CHECK_EQ((int)_conf.rules.size(), rulesBefore + 1);
	};

	IM_REGISTER_TEST(engine, testCategory(), "adopt dimensions")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/L-System", cfg::VoxEditShowLSystem));
		IM_CHECK(focusWindow(ctx, id));

		ctx->ItemClick("Adopt Dimensions");
		ctx->Yield();
		const voxel::RawVolume *volume = _sceneMgr->volume(_sceneMgr->activeNodeUUID());
		IM_CHECK(volume != nullptr);
		const voxel::Voxel wood = voxel::createVoxel(voxel::VoxelType::Generic, 1);
		const voxel::Region bounds = voxelgenerator::lsystem::evaluateRegion(_conf, wood);
		IM_CHECK(bounds.isValid());
		IM_CHECK(volume->region().containsRegion(bounds));
	};

	IM_REGISTER_TEST(engine, testCategory(), "cancel generation")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/L-System", cfg::VoxEditShowLSystem));
		IM_CHECK(focusWindow(ctx, id));

		ctx->ItemInputValue("Iterations", 5);
		ctx->ItemClick("###Ok");
		ctx->Yield(1);
		// the cancel button should appear while running
		if (_sceneMgr->lsystemRunning()) {
			ctx->ItemClick("###Cancel");
			ctx->Yield();
			IM_CHECK(!_sceneMgr->lsystemRunning());
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "copy paste rules")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/L-System", cfg::VoxEditShowLSystem));
		IM_CHECK(focusWindow(ctx, id));

		// copy current rules to clipboard
		ctx->MenuClick("Edit/Copy");
		ctx->Yield();

		// paste rules from clipboard
		ctx->MenuClick("Edit/Paste");
		ctx->Yield();
	};
}

} // namespace voxedit
