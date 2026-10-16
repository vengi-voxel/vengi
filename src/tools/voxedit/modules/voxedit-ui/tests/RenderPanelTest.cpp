/**
 * @file
 */

#include "../RenderPanel.h"
#include "../Viewport.h"
#include "../WindowTitles.h"
#include "TestUtil.h"
#include "command/CommandHandler.h"
#include "core/GLM.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxelpathtracer/PathTracerState.h"
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

namespace voxedit {

void RenderPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	auto prepareRenderPanel = [=](ImGuiTestContext *ctx) -> bool {
		voxelpathtracer::PathTracerParams &params = _pathTracer.state().params;
		params = voxelpathtracer::PathTracerParams();
		params.resolution = 64;
		voxelpathtracer::applyPathTracerQuality(params, voxelpathtracer::PathTracerQuality::Preview);
		IM_CHECK_RETV(prepareUITest(ctx), false);
		IM_CHECK_RETV(ensurePanelVisible(ctx, "View/Render", cfg::VoxEditShowRender), false);
		IM_CHECK_RETV(focusWindow(ctx, TITLE_RENDER), false);
		for (int i = 0; i < 120 && (isPreparing() || !_sceneReady); ++i) {
			ctx->Yield();
		}
		IM_CHECK_RETV(!_prepFuture.valid(), false);
		IM_CHECK_RETV(_sceneReady, false);
		return true;
	};

	IM_REGISTER_TEST(engine, testCategory(), "settings inputs")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareRenderPanel(ctx));
		voxelpathtracer::PathTracerState &state = _pathTracer.state();
		voxelpathtracer::PathTracerParams &params = state.params;

		ctx->MenuAction(ImGuiTestAction_Open, "Settings/Output");
		const int oldResolution = params.resolution;
		ctx->ItemInputValue("//$FOCUSED/Dimensions", oldResolution + 16);
		IM_CHECK(params.resolution == oldResolution + 16);

		ctx->MenuAction(ImGuiTestAction_Open, "Settings/Quality");
		const int oldSamples = params.samples;
		ctx->ItemInputValue("//$FOCUSED/Samples", oldSamples + 16);
		IM_CHECK(params.samples == oldSamples + 16);
	};

	IM_REGISTER_TEST(engine, testCategory(), "presets")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareRenderPanel(ctx));
		voxelpathtracer::PathTracerState &state = _pathTracer.state();
		voxelpathtracer::PathTracerParams &params = state.params;

		ctx->MenuClick("Settings/Quality presets/High quality");
		ctx->Yield();
		IM_CHECK_EQ(params.samples, 1024);
		IM_CHECK_EQ(params.bounces, 8);

		ctx->MenuClick("Settings/Quality presets/Preview");
		ctx->Yield();
		IM_CHECK_EQ(params.samples, 16);

		ctx->MenuClick("Settings/Quality presets/Reset all");
		ctx->Yield();
		voxelpathtracer::PathTracerParams defaults;
		IM_CHECK_EQ(params.samples, defaults.samples);
		IM_CHECK_EQ(params.bounces, defaults.bounces);
	};

	IM_REGISTER_TEST(engine, testCategory(), "hovered after focus")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareRenderPanel(ctx));
		ImGuiWindow *window = ImGui::FindWindowByName(TITLE_RENDER);
		IM_CHECK(window != nullptr);
		ctx->MouseMoveToPos(window->Rect().GetCenter());
		ctx->Yield();
		IM_CHECK(isHovered());
	};

	IM_REGISTER_TEST(engine, testCategory(), "camera persists without viewport")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareRenderPanel(ctx));
		ImGuiWindow *window = ImGui::FindWindowByName(TITLE_RENDER);
		IM_CHECK(window != nullptr);
		ctx->MouseMoveToPos(window->Rect().GetCenter());
		ctx->Yield();
		IM_CHECK(_camera.size().x > 0);
		IM_CHECK(_camera.size().y > 0);
		const glm::quat rotated = glm::normalize(
			glm::angleAxis(glm::radians(35.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * _camera.quaternion());
		_camera.setOrientation(rotated);
		_camera.update(0.0);
		const glm::quat expected = _camera.quaternion();
		ctx->Yield(5);
		IM_CHECK(glm::abs(glm::dot(_camera.quaternion(), expected)) > 0.99f);
	};

	IM_REGISTER_TEST(engine, testCategory(), "camera independent of viewport")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareRenderPanel(ctx));
		const int viewportId = viewportSceneMode(ctx, _app);
		IM_CHECK(viewportId != -1);
		Viewport *vp = viewportById(_app, viewportId);
		IM_CHECK(vp != nullptr);
		const core::String vid = Viewport::viewportId(viewportId);

		// Window operations need root references after prepareRenderPanel selected the render window.
		ctx->SetRef("");
		ctx->DockClear(TITLE_RENDER, vid.c_str(), nullptr);
		ctx->WindowResize(TITLE_RENDER, ImVec2(400, 400));
		ctx->WindowMove(TITLE_RENDER, ImVec2(40, 40));
		ctx->WindowResize(vid.c_str(), ImVec2(400, 400));
		ctx->WindowMove(vid.c_str(), ImVec2(460, 40));
		ctx->Yield(3);

		IM_CHECK(focusWindow(ctx, TITLE_RENDER));
		ctx->Yield();
		IM_CHECK(vp->isVisible());
		IM_CHECK(_camera.size().x > 0);
		const glm::quat renderBefore = _camera.quaternion();

		const glm::quat rotated = glm::normalize(
			glm::angleAxis(glm::radians(40.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * vp->camera().quaternion());
		vp->camera().setOrientation(rotated);
		vp->camera().update(0.0);
		ctx->Yield(5);
		IM_CHECK(glm::abs(glm::dot(_camera.quaternion(), renderBefore)) > 0.99f);
	};

	IM_REGISTER_TEST(engine, testCategory(), "wasd moves render camera")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareRenderPanel(ctx));
		const int viewportId = viewportSceneMode(ctx, _app);
		IM_CHECK(viewportId != -1);
		Viewport *vp = viewportById(_app, viewportId);
		IM_CHECK(vp != nullptr);

		IM_CHECK(focusWindow(ctx, TITLE_RENDER));
		ImGuiWindow *window = ImGui::FindWindowByName(TITLE_RENDER);
		IM_CHECK(window != nullptr);
		ctx->MouseMoveToPos(window->Rect().GetCenter());
		ctx->Yield(2);
		IM_CHECK(isHovered());
		IM_CHECK(_sceneMgr->activeCamera() == &_camera);

		const glm::vec3 renderBefore = _camera.worldPosition();
		const glm::vec3 viewportBefore = vp->camera().worldPosition();
		command::executeCommands("+move_forward 1 0.0");
		ctx->Yield(5);
		command::executeCommands("-move_forward 1 0.0");
		ctx->Yield();
		IM_CHECK(glm::length(_camera.worldPosition() - renderBefore) > 0.01f);
		IM_CHECK(glm::length(vp->camera().worldPosition() - viewportBefore) < 0.01f);
	};

	IM_REGISTER_TEST(engine, testCategory(), "shared sun angles")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareRenderPanel(ctx));
		_sunAngle->setVal("40.0 180.0 0.0");
		ctx->Yield(2);
		auto &params = _pathTracer.state().params;
		IM_CHECK_EQ(params.sunElevation, 40.0f);
		IM_CHECK_EQ(params.sunAzimuth, 180.0f);
		ctx->MenuAction(ImGuiTestAction_Open, "Settings/Lighting");
		ctx->ItemClick("//$FOCUSED/sunangle/Preset: Morning");
		ctx->Yield();
		IM_CHECK_STR_EQ(_sunAngle->strVal().c_str(), "15.00 45.00 0.00");
		IM_CHECK_EQ(params.sunElevation, 15.0f);
		IM_CHECK_EQ(params.sunAzimuth, 45.0f);
		ctx->MenuAction(ImGuiTestAction_Open, "Settings/Lighting");
		ctx->ItemClick("//$FOCUSED/sunangle/Preset: Evening");
		ctx->Yield();
		IM_CHECK_STR_EQ(_sunAngle->strVal().c_str(), "15.00 225.00 0.00");
		IM_CHECK_EQ(params.sunElevation, 15.0f);
		IM_CHECK_EQ(params.sunAzimuth, 225.0f);
	};

	IM_REGISTER_TEST(engine, testCategory(), "gpu preview")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareRenderPanel(ctx));
		const video::TexturePtr preview = _pathTracer.texture();
		IM_CHECK(preview != nullptr);
		IM_CHECK(preview->isLoaded());
		ImGuiWindow *window = ImGui::FindWindowByName(TITLE_RENDER);
		IM_CHECK(window != nullptr);
		bool displaysPreview = false;
		for (const ImDrawCmd &cmd : window->DrawList->CmdBuffer) {
			if (cmd.GetTexID() == (ImTextureID)preview->handle()) {
				displaysPreview = true;
			}
		}
		IM_CHECK(displaysPreview);
	};

	IM_REGISTER_TEST(engine, testCategory(), "resume prepared scene after hiding")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareRenderPanel(ctx));
		const uint64_t stamp = _sceneStamp;
		const video::TexturePtr texture = _pathTracer.texture();
		core::getVar(cfg::VoxEditShowRender)->setVal(false);
		ctx->Yield(3);
		IM_CHECK(_sceneReady);
		IM_CHECK(!_prepFuture.valid());
		IM_CHECK(!_pathTracer.started());

		IM_CHECK(ensurePanelVisible(ctx, "View/Render", cfg::VoxEditShowRender));
		IM_CHECK(focusWindow(ctx, TITLE_RENDER));
		ctx->Yield(2);
		IM_CHECK(_sceneReady);
		IM_CHECK(!_prepFuture.valid());
		IM_CHECK_EQ(_sceneStamp, stamp);
		IM_CHECK(_pathTracer.texture() == texture);
	};

	IM_REGISTER_TEST(engine, testCategory(), "defer uploads and reject stale scenes")->TestFunc = [=](ImGuiTestContext *ctx) {
		for (int replaceScene = 0; replaceScene < 2; ++replaceScene) {
			IM_CHECK(prepareRenderPanel(ctx));
			core::getVar(cfg::VoxEditShowRender)->setVal(false);
			ctx->Yield();
			_sceneReady = false;
			startScenePrep(_sceneMgr->sceneGraph(), _sceneMgr->calculateHash());
			for (int i = 0; i < 120 && !_prepFuture.ready(); ++i) {
				ctx->Yield();
			}
			ctx->Yield(2);
			IM_CHECK(_prepFuture.valid());
			IM_CHECK(_prepFuture.ready());
			IM_CHECK(!_sceneReady);
			IM_CHECK(!_pathTracer.started());

			if (replaceScene) {
				IM_CHECK(_sceneMgr->newScene(true, "replacement", voxel::Region(0, 1)));
				IM_CHECK_NE(_prepStamp, _sceneMgr->calculateHash());
			}
			IM_CHECK(ensurePanelVisible(ctx, "View/Render", cfg::VoxEditShowRender));
			IM_CHECK(focusWindow(ctx, TITLE_RENDER));
			for (int i = 0; i < 120 && (isPreparing() || !_sceneReady); ++i) {
				ctx->Yield();
			}
			IM_CHECK(!_prepFuture.valid());
			IM_CHECK(_sceneReady);
			IM_CHECK_EQ(_sceneStamp, _sceneMgr->calculateHash());
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "mouse rotate render camera")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareRenderPanel(ctx));
		ImGuiWindow *window = ImGui::FindWindowByName(TITLE_RENDER);
		IM_CHECK(window != nullptr);
		const ImVec2 center = window->Rect().GetCenter();
		ctx->MouseMoveToPos(center);
		ctx->Yield(2);
		IM_CHECK(isHovered());
		IM_CHECK(_sceneMgr->activeCamera() == &_camera);

		const glm::quat before = _camera.quaternion();
		command::executeCommands("+camera_rotate 1 0.0");
		ctx->MouseMoveToPos(ImVec2(center.x + 80.0f, center.y));
		ctx->Yield(5);
		command::executeCommands("-camera_rotate 1 0.0");
		ctx->Yield();
		IM_CHECK(glm::abs(glm::dot(_camera.quaternion(), before)) < 0.999f);
	};
}

} // namespace voxedit
