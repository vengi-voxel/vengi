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
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(newTemplateScene(ctx, "##templates/##River"));
		IM_CHECK(changeViewMode(ctx, ViewMode::RedAlert2));
		IM_CHECK(focusWindow(ctx, id));

		// remove all normals first
		ctx->MenuClick("File/Remove all normals");
		ctx->Yield();

		// calculate normals with default (Flat) mode
		ctx->MenuClick("File/Auto normals/Calculate normals");
		ctx->Yield();
	};

	// export normal palette
	IM_REGISTER_TEST(engine, testCategory(), "export normal palette")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(changeViewMode(ctx, ViewMode::RedAlert2));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Export");
		ctx->Yield();
		IM_CHECK(saveFile(ctx, "normalpalette-export.png"));
	};

	// change longitude and latitude and verify values change
	IM_REGISTER_TEST(engine, testCategory(), "longitude latitude")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(changeViewMode(ctx, ViewMode::RedAlert2));
		IM_CHECK(focusWindow(ctx, id));

		const glm::vec3 beforeLon = _targetNormal;
		changeSlider(ctx, "Longitude", true);
		ctx->Yield();
		const glm::vec3 afterLon = _targetNormal;
		IM_CHECK(afterLon.x != beforeLon.x || afterLon.z != beforeLon.z);

		changeSlider(ctx, "Longitude", false);

		const glm::vec3 beforeLat = _targetNormal;
		changeSlider(ctx, "Latitude", true);
		ctx->Yield();
		const glm::vec3 afterLat = _targetNormal;
		IM_CHECK(afterLat.y != beforeLat.y || afterLat.x != beforeLat.x || afterLat.z != beforeLat.z);

		changeSlider(ctx, "Latitude", false);
	};

	IM_REGISTER_TEST(engine, testCategory(), "flip buttons")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(changeViewMode(ctx, ViewMode::RedAlert2));
		IM_CHECK(focusWindow(ctx, id));

		const glm::vec3 before = _targetNormal;
		ctx->ItemClick("##flipnormal/X");
		ctx->Yield();
		IM_CHECK(_targetNormal.x != before.x || _targetNormal.y != before.y || _targetNormal.z != before.z);

		ctx->ItemClick("##flipnormal/Y");
		ctx->Yield();
		ctx->ItemClick("##flipnormal/Z");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "rotate buttons")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(changeViewMode(ctx, ViewMode::RedAlert2));
		IM_CHECK(focusWindow(ctx, id));

		const glm::vec3 before = _targetNormal;
		ctx->ItemClick("##rotnormal/X");
		ctx->Yield();
		IM_CHECK(_targetNormal.x != before.x || _targetNormal.y != before.y || _targetNormal.z != before.z);

		ctx->ItemClick("##rotnormal/Y");
		ctx->Yield();
		ctx->ItemClick("##rotnormal/Z");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "direct normal input")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(changeViewMode(ctx, ViewMode::RedAlert2));
		IM_CHECK(focusWindow(ctx, id));

		// InputFloat3 "Normal" has 3 sub-inputs accessible via $$0, $$1, $$2
		ctx->ItemInputValue("Normal/$$0", 1.0f);
		ctx->Yield();
		ctx->ItemInputValue("Normal/$$1", 0.0f);
		ctx->Yield();
		ctx->ItemInputValue("Normal/$$2", 0.0f);
		ctx->Yield();
	};
}

} // namespace voxedit
