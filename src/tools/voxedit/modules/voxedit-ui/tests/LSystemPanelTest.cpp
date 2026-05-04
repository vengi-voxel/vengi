/**
 * @file
 */

#include "../LSystemPanel.h"
#include "../ViewMode.h"
#include "TestUtil.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxedit {

void LSystemPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "default rule")->TestFunc = [=](ImGuiTestContext *ctx) {
		if (!viewModeLSystemPanel(core::getVar(cfg::VoxEditViewMode)->intVal())) {
			return;
		}
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(resetScene(ctx, _sceneMgr));
		const int activeNode = _sceneMgr->sceneGraph().activeNode();
		const voxel::RawVolume *volume = _sceneMgr->volume(activeNode);
		IM_CHECK(volume != nullptr);

		ctx->ItemInputValue("Iterations", 1);
		ctx->ItemClick("###Ok");
		ctx->Yield(1); // give a chance to execute at least one step of the lsystem generation
		_sceneMgr->lsystemAbort();
		IM_CHECK(voxelutil::countVoxels(*volume) > 0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "template selection")->TestFunc = [=](ImGuiTestContext *ctx) {
		if (!viewModeLSystemPanel(core::getVar(cfg::VoxEditViewMode)->intVal())) {
			return;
		}
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(resetScene(ctx, _sceneMgr));

		// verify the Templates combo exists and has entries
		if (!_templates.empty()) {
			const core::String &firstName = _templates[0].name;
			ctx->ComboClick(core::String::format("Templates/%s", firstName.c_str()).c_str());
			ctx->Yield();
			// after selecting a template, the axiom should be filled
			IM_CHECK(!_conf.axiom.empty());
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "parameter changes")->TestFunc = [=](ImGuiTestContext *ctx) {
		if (!viewModeLSystemPanel(core::getVar(cfg::VoxEditViewMode)->intVal())) {
			return;
		}
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(resetScene(ctx, _sceneMgr));

		ctx->ItemInputValue("Angle", 45.0f);
		IM_CHECK_EQ((int)glm::degrees(_conf.angle), 45);

		ctx->ItemInputValue("Length", 5.0f);
		IM_CHECK_EQ((int)_conf.length, 5);

		ctx->ItemInputValue("Iterations", 3);
		IM_CHECK_EQ(_conf.iterations, 3);
	};

	IM_REGISTER_TEST(engine, testCategory(), "add delete rule")->TestFunc = [=](ImGuiTestContext *ctx) {
		if (!viewModeLSystemPanel(core::getVar(cfg::VoxEditViewMode)->intVal())) {
			return;
		}
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(resetScene(ctx, _sceneMgr));

		const int rulesBefore = (int)_conf.rules.size();
		ctx->ItemClick("Add Rule###Add Rule");
		ctx->Yield();
		IM_CHECK_EQ((int)_conf.rules.size(), rulesBefore + 1);
	};

	IM_REGISTER_TEST(engine, testCategory(), "adopt dimensions")->TestFunc = [=](ImGuiTestContext *ctx) {
		if (!viewModeLSystemPanel(core::getVar(cfg::VoxEditViewMode)->intVal())) {
			return;
		}
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(resetScene(ctx, _sceneMgr));

		ctx->ItemClick("Adopt Dimensions");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "cancel generation")->TestFunc = [=](ImGuiTestContext *ctx) {
		if (!viewModeLSystemPanel(core::getVar(cfg::VoxEditViewMode)->intVal())) {
			return;
		}
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(resetScene(ctx, _sceneMgr));

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
		if (!viewModeLSystemPanel(core::getVar(cfg::VoxEditViewMode)->intVal())) {
			return;
		}
		IM_CHECK(focusWindow(ctx, id));
		IM_CHECK(resetScene(ctx, _sceneMgr));

		// copy current rules to clipboard
		ctx->MenuClick("Edit/Copy");
		ctx->Yield();

		// paste rules from clipboard
		ctx->MenuClick("Edit/Paste");
		ctx->Yield();
	};
}

} // namespace voxedit
