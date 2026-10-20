/**
 * @file
 */

#include "RenderPanel.h"
#include "SunAngleSettings.h"
#include "app/Async.h"
#include "core/ArrayLength.h"
#include "core/GLM.h"
#include "core/ProgressScope.h"
#include "core/SharedPtr.h"
#include "io/FileStream.h"
#include "io/FormatDescription.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphUtil.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/ScopedPanel.h"
#include "ui/ScopedStyle.h"
#include "ui/dearimgui/ImGuizmo.h"
#include "video/Camera.h"
#include "video/Texture.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/Region.h"
#include "voxelpathtracer/PathTracer.h"
#include "voxelpathtracer/PathTracerConfig.h"
#include "voxelpathtracer/PathTracerState.h"
#include "VoxelpathtracerShaderConstants.h"
#include "voxelrender/RenderUtil.h"
#include <glm/gtc/type_ptr.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/matrix_decompose.hpp>

namespace voxedit {

namespace {

static const char *BackgroundItems[] = {"Sky", "Solid", "Transparent"};

} // namespace

bool RenderPanel::init() {
	_sunAngle = core::getVar(cfg::VoxEditSunAngle);
	return _pathTracer.init();
}

void RenderPanel::applySceneSun() {
	if (!_sunAngle) {
		return;
	}
	glm::vec3 sunAngle(35.0f, 135.0f, 0.0f);
	_sunAngle->vec3Val(&sunAngle[0]);
	voxelpathtracer::PathTracerState &state = _pathTracer.state();
	voxelpathtracer::PathTracerParams &params = state.params;
	if (glm::abs(params.sunElevation - sunAngle.x) > 0.01f || glm::abs(params.sunAzimuth - sunAngle.y) > 0.01f) {
		params.sunElevation = sunAngle.x;
		params.sunAzimuth = sunAngle.y;
		voxelpathtracer::PathTracerConfig::saveParams(params);
		_pathTracer.resetAccumulation();
	}
}

void RenderPanel::renderSettings(const scenegraph::SceneGraph &sceneGraph) {
	voxelpathtracer::PathTracerState &pstate = _pathTracer.state();
	voxelpathtracer::PathTracerParams &state = pstate.params;
	// Pick up Options panel / -set changes before editing the live params.
	voxelpathtracer::PathTracerConfig::loadParams(state);
	applySceneSun();
	int changed = 0;

	const float itemWidth = ImGui::GetFontSize() * 10.0f;

	if (ImGui::BeginIconMenu(ICON_LC_SPARKLES, _("Quality presets"))) {
		if (ImGui::IconMenuItem(ICON_LC_ROTATE_CCW, _("Reset all"))) {
			voxelpathtracer::applyPathTracerQuality(state, voxelpathtracer::PathTracerQuality::Default);
			++changed;
		}
		ImGui::TooltipTextUnformatted(_("Restore default path tracer settings"));
		if (ImGui::IconMenuItem(ICON_LC_SPARKLES, _("High quality"))) {
			voxelpathtracer::applyPathTracerQuality(state, voxelpathtracer::PathTracerQuality::High);
			++changed;
		}
		ImGui::TooltipTextUnformatted(_("Path tracing with high sample and bounce counts, voxel edge, and bloom"));
		if (ImGui::IconMenuItem(ICON_LC_BOX, _("Preview"))) {
			voxelpathtracer::applyPathTracerQuality(state, voxelpathtracer::PathTracerQuality::Preview);
			++changed;
		}
		ImGui::TooltipTextUnformatted(_("Fast preview for checking lighting and materials"));
		ImGui::EndMenu();
	}

	if (ImGui::BeginIconMenu(ICON_LC_GAUGE, _("Quality"))) {
		ImGui::PushItemWidth(itemWidth);
		changed += ImGui::InputIntWithButtons(_("Samples"), &state.samples, 16, 4096);
		ImGui::TooltipTextUnformatted(_("Per-pixel samples. Higher values reduce noise but take longer."));
		changed += ImGui::SliderInt(_("Bounces"), &state.bounces, 1, shader::VoxelpathtracerShaderConstants::getMaxBounces());
		ImGui::TooltipTextUnformatted(_("Maximum light bounces. Increase for glass."));
		changed += ImGui::SliderInt(_("Samples per pass"), &state.samplesThisPass, 1,
									shader::VoxelpathtracerShaderConstants::getMaxSamplesThisPass());
		ImGui::TooltipTextUnformatted(_("Samples computed per frame"));
		changed += ImGui::SliderFloat(_("Voxel edge"), &state.voxelEdge, 0.0f, 1.0f);
		ImGui::TooltipTextUnformatted(_("Fillet exposed voxel corners. 0 is a hard cube."));
		changed += ImGui::SliderFloat(_("Render scale"), &state.renderScale, 0.25f, 1.0f);
		ImGui::TooltipTextUnformatted(_("Trace at a fraction of the output resolution, then upsample."));
		ImGui::PopItemWidth();
		ImGui::EndMenu();
	}

	if (ImGui::BeginIconMenu(ICON_LC_IMAGE, _("Output"))) {
		ImGui::PushItemWidth(itemWidth);
		changed += ImGui::InputIntWithButtons(_("Dimensions"), &state.resolution);
		ImGui::TooltipTextUnformatted(_("Output image size in pixels (square)"));
		ImGui::SliderFloat(_("Exposure"), &state.exposure, -5.0f, 5.0f);
		ImGui::TooltipTextUnformatted(_("Exposure compensation in stops for tonemapping."));
		changed += ImGui::Combo(_("Tone mapping"), &state.toneMapping, voxelrender::ToneMappingItems,
								voxelrender::ToneMappingMax);
		ImGui::SliderFloat(_("Contrast"), &state.contrast, 0.5f, 1.5f);
		ImGui::SliderFloat(_("Saturation"), &state.saturation, 0.0f, 2.0f);
		ImGui::SliderFloat(_("Vignette"), &state.vignette, 0.0f, 1.0f);
		changed += ImGui::SliderFloat(_("Bloom"), &state.bloom, 0.0f, 2.0f);
		ImGui::TooltipTextUnformatted(_("Threshold blur of bright emit and sun highlights."));
		ImGui::PopItemWidth();
		ImGui::EndMenu();
	}

	if (ImGui::BeginIconMenu(ICON_LC_CAMERA, _("Camera"))) {
		ImGui::PushItemWidth(itemWidth);
		changed += ImGui::Checkbox(_("Depth of field"), &state.depthOfField);
		changed += ImGui::SliderFloat(_("Aperture"), &state.aperture, 0.0f, 0.5f);
		ImGui::TooltipTextUnformatted(_("Lens aperture for depth of field. 0 means pinhole (no DOF)."));
		changed += ImGui::SliderFloat(_("Focus distance"), &state.focusDistance, 1.0f, 256.0f);
		ImGui::PopItemWidth();
		ImGui::EndMenu();
	}

	if (ImGui::BeginIconMenu(ICON_LC_SUN, _("Lighting"))) {
		if (sunAngleSettings(_sunAngle, true)) {
			applySceneSun();
			++changed;
		}
		ImGui::Separator();
		ImGui::PushItemWidth(itemWidth);
		changed += ImGui::SliderFloat(_("Sun intensity"), &state.sunIntensity, 0.0f, 10.0f);
		changed += ImGui::SliderFloat(_("Sun softness"), &state.sunSoftness, 0.05f, 20.0f);
		ImGui::TooltipTextUnformatted(_("Sun disk size in degrees."));
		changed += ImGui::ColorEdit3(_("Sun color"), &state.sunColor.x);
		changed += ImGui::ColorEdit3(_("Sky top"), &state.skyTop.x);
		changed += ImGui::ColorEdit3(_("Sky horizon"), &state.skyHorizon.x);
		changed += ImGui::SliderFloat(_("Sky intensity"), &state.skyIntensity, 0.0f, 4.0f);
		ImGui::Separator();
		changed += ImGui::Checkbox(_("Floor"), &state.floorEnabled);
		changed += ImGui::ColorEdit3(_("Floor color"), &state.floorColor.x);
		changed += ImGui::SliderFloat(_("Floor Y"), &state.floorY, -256.0f, 256.0f);
		ImGui::Separator();
		changed += ImGui::Combo(_("Background"), &state.background, BackgroundItems, lengthof(BackgroundItems));
		changed += ImGui::ColorEdit3(_("Background color"), &state.backgroundColor.x);
		changed += ImGui::SliderFloat(_("Fog density"), &state.fogDensity, 0.0f, 0.05f);
		changed += ImGui::ColorEdit3(_("Fog color"), &state.fogColor.x);
		changed += ImGui::SliderFloat(_("Atmosphere"), &state.atmosphere, 0.0f, 0.05f);
		ImGui::TooltipTextUnformatted(_("World haze that brightens toward the sun."));
		ImGui::PopItemWidth();
		ImGui::EndMenu();
	}

	// Keep cvars in sync even for post-process-only tweaks that do not reset accumulation.
	voxelpathtracer::PathTracerConfig::saveParams(state);
	if (changed > 0) {
		_pathTracer.resetAccumulation();
	}
}

void RenderPanel::renderMenuBar(const scenegraph::SceneGraph &sceneGraph) {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginIconMenu(ICON_LC_SETTINGS, _("Settings"))) {
			renderSettings(sceneGraph);
			ImGui::EndMenu();
		}
		if (_currentSample > 0) {
			if (ImGui::IconMenuItem(ICON_LC_SAVE, _("Save image"))) {
				const image::ImagePtr snapshot = _pathTracer.image();
				if (!snapshot || !snapshot->isLoaded()) {
					ImGui::EndMenuBar();
					return;
				}
				_app->saveDialog(
					[=](const core::String &file, const io::FormatDescription *desc) {
						const io::FilePtr &filePtr = _app->filesystem()->open(file, io::FileMode::SysWrite);
						io::FileStream stream(filePtr);
						image::writePNG(snapshot, stream);
					},
					{}, io::format::images(), "render.png");
			}
		}
		const voxelpathtracer::PathTracerState &state = _pathTracer.state();
		ImGui::Text(_("Sample %i / %i"), _currentSample, state.params.samples);
		if (state.volumeTruncated) {
			ImGui::TextUnformatted(_("Volume truncated to GPU 3D texture / memory limits"));
		}
		ImGui::EndMenuBar();
	}
}

void RenderPanel::renderCameraManipulator(video::Camera &camera, float headerSize, bool sceneMode) {
	if (camera.size().x <= 0 || camera.size().y <= 0) {
		return;
	}
	ImVec2 position = ImGui::GetWindowPos();
	const ImVec2 size = ImVec2(128, 128);
	const ImVec2 available = ImGui::GetContentRegionAvail();
	const float contentRegionWidth = available.x + ImGui::GetCursorPosX();
	position.x += contentRegionWidth - size.x;
	position.y += headerSize;
	const ImU32 backgroundColor = 0;
	const float length = camera.targetDistance();

	glm::mat4 viewMatrix = camera.viewMatrix();
	float *viewPtr = glm::value_ptr(viewMatrix);

	ImGuizmo::PushID("pathtracer");
	ImGuizmo::SetDrawlist();
	ImGuizmo::SetWindow();
	ImGuizmo::SetOrthographic(camera.isOrthographic());
	if (sceneMode) {
		ImGuizmo::ViewManipulate(viewPtr, length, position, size, backgroundColor);
	} else {
		const float *projPtr = glm::value_ptr(camera.projectionMatrix());
		const ImGuizmo::OPERATION operation = (ImGuizmo::OPERATION)0;
		glm::mat4 transformMatrix = glm::mat4(1.0f);
		float *matrixPtr = glm::value_ptr(transformMatrix);
		const ImGuizmo::MODE mode = ImGuizmo::MODE::LOCAL;
		ImGuizmo::ViewManipulate(viewPtr, projPtr, operation, mode, matrixPtr, length, position, size, backgroundColor);
	}
	ImGuizmo::PopID();
	if (viewMatrix != camera.viewMatrix()) {
		glm::vec3 scale;
		glm::vec3 translation;
		glm::quat orientation;
		glm::vec3 skew;
		glm::vec4 perspective;
		glm::decompose(viewMatrix, scale, orientation, translation, skew, perspective);
		camera.setOrientation(orientation);
		camera.update(0.0);
	}
}

void RenderPanel::ensureTraceCamera(const scenegraph::SceneGraph &sceneGraph) {
	if (_camera.size().x > 0 && _camera.size().y > 0) {
		return;
	}
	if (const video::Camera *src = _sceneMgr->activeCamera()) {
		if (src->size().x > 0 && src->size().y > 0) {
			_camera = *src;
			return;
		}
	}
	voxel::Region region = sceneGraph.sceneRegion();
	if (!region.isValid()) {
		region = voxel::Region(0, 0, 0, 31, 31, 31);
	}
	if (_camera.size().x <= 0 || _camera.size().y <= 0) {
		_camera.setSize(glm::ivec2(512, 512));
	}
	_camera.setMode(video::CameraMode::Perspective);
	const glm::vec3 dim = glm::vec3(region.getDimensionsInVoxels());
	const float farPlane = glm::max(32.0f, glm::length(dim) * 4.0f);
	voxelrender::configureCamera(_camera, region, voxelrender::SceneCameraMode::Free, farPlane);
}

void RenderPanel::renderPrepProgress() {
	const ImVec2 windowSize = ImGui::GetWindowSize();
	const float barWidth = ImGui::GetFontSize() * 24.0f;
	const float barHeight = ImGui::GetFrameHeight();
	const float spacing = ImGui::GetStyle().ItemSpacing.y;
	const char *title = _("Preparing render");

	ImVec2 titleSize;
	{
		ui::ScopedStyle style;
		style.pushFontSize(imguiApp()->bigFontSize());
		titleSize = ImGui::CalcTextSize(title);
	}
	const float blockHeight = titleSize.y + spacing + barHeight;
	const float startY = (windowSize.y - blockHeight) * 0.5f;

	{
		ui::ScopedStyle style;
		style.pushFontSize(imguiApp()->bigFontSize());
		ImGui::SetCursorPosX((windowSize.x - titleSize.x) * 0.5f);
		ImGui::SetCursorPosY(startY);
		ImGui::TextUnformatted(title);
	}

	ImGui::SetCursorPosX((windowSize.x - barWidth) * 0.5f);
	ImGui::SetCursorPosY(startY + titleSize.y + spacing);
	const float progress = _prepProgress.progress();
	const core::String progressText = _prepProgress.text();
	char overlay[96];
	if (progressText.empty()) {
		core::String::formatBuf(overlay, sizeof(overlay), "%.0f%%", (double)(progress * 100.0f));
	} else {
		core::String::formatBuf(overlay, sizeof(overlay), "%s %.0f%%", progressText.c_str(),
								(double)(progress * 100.0f));
	}
	ImGui::ProgressBar(progress, ImVec2(barWidth, 0.0f), overlay);
}

void RenderPanel::cancelScenePrep() {
	if (_prepFuture.valid()) {
		_prepFuture.wait();
		_prepFuture = {};
	}
	_prepProgress.reset();
	_prepStamp = 0;
}

bool RenderPanel::pollScenePrep(const scenegraph::SceneGraph &sceneGraph) {
	if (!_prepFuture.valid()) {
		return false;
	}
	if (!_prepFuture.ready()) {
		return true;
	}

	voxelpathtracer::PreparedGpuScene prepared = _prepFuture.get();
	_prepFuture = {};
	const uint64_t finishedStamp = _prepStamp;
	_prepStamp = 0;
	_prepProgress.reset();

	const uint64_t stamp = _sceneMgr->calculateHash();
	if (finishedStamp != stamp) {
		prepared.free();
		return false;
	}
	if (!_pathTracer.startPrepared(core::move(prepared), sceneGraph, &_camera)) {
		_sceneReady = false;
		return false;
	}
	_sceneReady = true;
	_sceneStamp = stamp;
	return false;
}

void RenderPanel::startScenePrep(const scenegraph::SceneGraph &sceneGraph, uint64_t stamp) {
	if (_prepFuture.valid()) {
		return;
	}

	// Shared snapshot: app::async stores the lambda in core::Function, which must be copyable.
	core::SharedPtr<scenegraph::SceneGraph> snapshot = core::make_shared<scenegraph::SceneGraph>();
	scenegraph::copySceneGraph(*snapshot.get(), sceneGraph);
	const int maxTextureAxis = voxelpathtracer::PathTracer::maxTextureAxis();
	_prepStamp = stamp;
	_prepProgress.reset();
	_prepProgress.setText(_("Preparing"));
	core::SharedProgress *progress = &_prepProgress;
	_prepFuture = app::async([snapshot, maxTextureAxis, progress]() {
		core::ProgressScope scope(*progress);
		return voxelpathtracer::prepareGpuScene(*snapshot.get(), maxTextureAxis);
	});
}

void RenderPanel::updateSceneState(const scenegraph::SceneGraph &sceneGraph) {
	if (_sceneMgr->isLoading()) {
		_pathTracer.stop();
		_sceneReady = false;
		return;
	}

	const uint64_t stamp = _sceneMgr->calculateHash();
	if (pollScenePrep(sceneGraph)) {
		return;
	}

	if (!_sceneReady || stamp != _sceneStamp) {
		startScenePrep(sceneGraph, stamp);
		return;
	}

	if (!_pathTracer.started()) {
		_pathTracer.resume();
	}
	_pathTracer.applyCamera(&_camera);
}

void RenderPanel::update(const char *id, const scenegraph::SceneGraph &sceneGraph) {
	core_trace_scoped(RenderPanel);
	static ui::ScopedPanel panel(cfg::VoxEditShowRender);
	const core::String &title = makeTitle(ICON_LC_IMAGE, _("Render"), id);
	ui::ScopedPanel::Scope scope =
		panel.begin(title.c_str(), ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_MenuBar |
						ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	_hovered = false;
	if (!scope) {
		// Keep the prepared scene and defer uploads until the panel is visible again.
		_pathTracer.stop();
		return;
	}

	renderMenuBar(sceneGraph);
	applySceneSun();

	ensureTraceCamera(sceneGraph);

	const ImVec2 cursorPos = ImGui::GetCursorPos();
	const float headerSize = cursorPos.y;
	const ImVec2 contentSize = ImGui::GetContentRegionAvail();
	if (contentSize.x > 0.0f && contentSize.y > 0.0f) {
		_camera.setSize(glm::ivec2((int)contentSize.x, (int)contentSize.y));
	}
	if (_camera.size().x > 0 && _camera.size().y > 0) {
		_camera.update(0.0);
	}

	updateSceneState(sceneGraph);

	if (isPreparing()) {
		renderPrepProgress();
		return;
	}

	_pathTracer.update(&_currentSample);
	const video::TexturePtr texture = _pathTracer.texture();
	if (texture && texture->isLoaded() && contentSize.x > 0.0f && contentSize.y > 0.0f) {
		const glm::vec4 &uv = video::framebufferUV();
		ImGui::SetCursorPos(cursorPos);
		ImGui::Image(texture->handle(), contentSize, glm::vec2(uv.x, uv.y), glm::vec2(uv.z, uv.w));
	}

	renderCameraManipulator(_camera, headerSize, true);

	_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
									  ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_DockHierarchy);
	if (_hovered) {
		_sceneMgr->setActiveCamera(&_camera, false);
		updateCameraInput(headerSize);
	}
}

void RenderPanel::updateCameraInput(float headerSize) {
	if (ImGuizmo::IsViewManipulateHovered()) {
		return;
	}
	const ImVec2 windowPos = ImGui::GetWindowPos();
	const int mouseX = (int)(ImGui::GetIO().MousePos.x - windowPos.x);
	const int mouseY = (int)((ImGui::GetIO().MousePos.y - windowPos.y) - headerSize);
	_sceneMgr->setMousePos(mouseX, mouseY);
}

void RenderPanel::shutdown() {
	cancelScenePrep();
	_pathTracer.shutdown();
}

} // namespace voxedit
