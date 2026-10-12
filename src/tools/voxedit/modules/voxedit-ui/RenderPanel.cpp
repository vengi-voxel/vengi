/**
 * @file
 */

#include "RenderPanel.h"
#include "core/SharedPtr.h"
#include "core/Trace.h"
#include "core/GLM.h"
#include "io/FileStream.h"
#include "scenegraph/SceneGraph.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/ScopedPanel.h"
#include "ui/dearimgui/ImGuizmo.h"
#include "video/Camera.h"
#include "video/Texture.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/Region.h"
#include "voxelpathtracer/PathTracer.h"
#include "voxelpathtracer/PathTracerState.h"
#include "voxelrender/RenderUtil.h"
#include <glm/gtc/type_ptr.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/matrix_decompose.hpp>

namespace voxedit {

bool RenderPanel::init() {
	_texture = video::createEmptyTexture("pathtracer");
	return true;
}

void RenderPanel::renderSettings(const scenegraph::SceneGraph &sceneGraph) {
	voxelpathtracer::PathTracerState &state = _pathTracer.state();
	voxelpathtracer::PathTracerParams &params = state.params;
	int changed = 0;

	const float itemWidth = ImGui::GetFontSize() * 10.0f;

	if (ImGui::BeginIconMenu(ICON_LC_SPARKLES, _("Presets"))) {
		if (ImGui::IconMenuItem(ICON_LC_ROTATE_CCW, _("Reset all"))) {
			params = voxelpathtracer::PathTracerParams();
			++changed;
		}
		ImGui::TooltipTextUnformatted(_("Restore default path tracer settings"));
		if (ImGui::IconMenuItem(ICON_LC_SPARKLES, _("High quality"))) {
			params = voxelpathtracer::PathTracerParams();
			params.sampler = yocto::trace_sampler_type::path;
			params.samples = 1024;
			params.bounces = 64;
			++changed;
		}
		ImGui::TooltipTextUnformatted(_("Path tracing with high sample and bounce counts"));
		if (ImGui::IconMenuItem(ICON_LC_BOX, _("Geometry preview"))) {
			params = voxelpathtracer::PathTracerParams();
			params.sampler = yocto::trace_sampler_type::eyelight;
			params.samples = 16;
			++changed;
		}
		ImGui::TooltipTextUnformatted(_("Fast eyelight preview for checking geometry"));
		ImGui::EndMenu();
	}

	if (ImGui::BeginIconMenu(ICON_LC_GAUGE, _("Quality"))) {
		ImGui::PushItemWidth(itemWidth);
		changed += ImGui::ComboItems(_("Tracer"), (int *)&params.sampler, yocto::trace_sampler_names);
		changed += ImGui::InputIntWithButtons(_("Samples"), &params.samples, 16, 4096);
		ImGui::TooltipTextUnformatted(_("Per-pixel samples. Higher values reduce noise but take longer."));
		changed += ImGui::SliderInt(_("Bounces"), &params.bounces, 1, 128);
		ImGui::TooltipTextUnformatted(_("Maximum light bounces. Increase for glass and volumes."));
		changed += ImGui::SliderFloat(_("Clamp"), &params.clamp, 10, 1000);
		ImGui::TooltipTextUnformatted(_("Remove high-energy fireflies"));
		ImGui::Separator();
		changed += ImGui::SliderInt(_("Preview ratio"), &params.pratio, 1, 64);
		ImGui::TooltipTextUnformatted(_("Lower resolution ratio used while samples accumulate"));
		changed += ImGui::SliderInt(_("Batch"), &params.batch, 1, 16);
		ImGui::TooltipTextUnformatted(_("Samples computed per update step"));
		ImGui::PopItemWidth();
		ImGui::EndMenu();
	}

	if (ImGui::BeginIconMenu(ICON_LC_IMAGE, _("Output"))) {
		ImGui::PushItemWidth(itemWidth);
		changed += ImGui::InputIntWithButtons(_("Dimensions"), &params.resolution);
		ImGui::TooltipTextUnformatted(_("Output image size in pixels (square)"));
		changed += ImGui::Checkbox(_("Filter"), &params.tentfilter);
		ImGui::TooltipTextUnformatted(_("Apply a linear filter to the image pixels"));
		changed += ImGui::Checkbox(_("Denoise"), &params.denoise);
		ImGui::Separator();
		ImGui::SliderFloat(_("Exposure"), &state.exposure, -5.0f, 5.0f);
		ImGui::TooltipTextUnformatted(_("Exposure compensation in stops for tonemapping."));
		ImGui::Checkbox(_("Filmic"), &state.filmic);
		ImGui::TooltipTextUnformatted(_("Use filmic tonemapping for softer highlight rolloff."));
		ImGui::PopItemWidth();
		ImGui::EndMenu();
	}

	if (ImGui::BeginIconMenu(ICON_LC_CAMERA, _("Camera"))) {
		ImGui::PushItemWidth(itemWidth);
		if (!state.scene.camera_names.empty()) {
			changed += ImGui::ComboItems(_("Camera"), &params.camera, state.scene.camera_names);
		}
		changed += ImGui::SliderFloat(_("Aperture"), &state.aperture, 0.0f, 0.5f);
		ImGui::TooltipTextUnformatted(_("Lens aperture for depth of field. 0 means pinhole (no DOF)."));
		ImGui::PopItemWidth();
		ImGui::EndMenu();
	}

	if (ImGui::BeginIconMenu(ICON_LC_SUN, _("Lighting"))) {
		ImGui::PushItemWidth(itemWidth);
		changed += ImGui::SliderFloat(_("Sun intensity"), &state.sunIntensity, 0.0f, 10.0f);
		changed += ImGui::SliderFloat(_("Sun area"), &state.sunArea, 0.0f, 5.0f);
		ImGui::TooltipTextUnformatted(_("Sun disk size. 1.0 is about 43.5 degrees."));
		changed += ImGui::SliderAngle(_("Sun elevation"), &state.sunElevation, 0.0f, 90.0f);
		changed += ImGui::SliderAngle(_("Sun azimuth"), &state.sunAzimuth, 0.0f, 360.0f);
		changed += ImGui::Checkbox(_("Sun disk"), &state.sunDisk);
		ImGui::TooltipTextUnformatted(_("Show visible sun disk in the sky."));
		ImGui::Separator();
		changed += ImGui::Checkbox(_("Hide environment"), &params.envhidden);
		ImGui::TooltipTextUnformatted(_("Removes the environment map from the camera rays."));
		ImGui::PopItemWidth();
		ImGui::EndMenu();
	}

	if (ImGui::BeginIconMenu(ICON_LC_WRENCH, _("Advanced"))) {
		changed += ImGui::Checkbox(_("No caustics"), &params.nocaustics);
		ImGui::TooltipTextUnformatted(_("Removes certain paths that cause caustics"));
		changed += ImGui::Checkbox(_("High Quality BVH"), &params.highqualitybvh);
		ImGui::TooltipTextUnformatted(_("High quality bounding volume hierarchy"));
		ImGui::EndMenu();
	}

	if (changed > 0) {
		_pathTracer.restart(sceneGraph, _sceneMgr->activeCamera());
	}
}

void RenderPanel::renderMenuBar(const scenegraph::SceneGraph &sceneGraph) {
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginIconMenu(ICON_LC_SETTINGS, _("Settings"))) {
			renderSettings(sceneGraph);
			ImGui::EndMenu();
		}
		if (_image && _image->isLoaded()) {
			if (ImGui::IconMenuItem(ICON_LC_SAVE, _("Save image"))) {
				_app->saveDialog(
					[=](const core::String &file, const io::FormatDescription *desc) {
						const io::FilePtr &filePtr = _app->filesystem()->open(file, io::FileMode::SysWrite);
						io::FileStream stream(filePtr);
						image::writePNG(_image, stream);
					},
					{}, io::format::images(), "render.png");
			}
		}
		if (_pathTracer.started()) {
			if (ImGui::IconMenuItem(ICON_LC_REFRESH_CW, _("Sync camera"))) {
				_pathTracer.restart(sceneGraph, _sceneMgr->activeCamera());
			}
			ImGui::TooltipTextUnformatted(_("Restart with the current viewport camera"));
			if (ImGui::IconMenuItem(ICON_LC_CIRCLE_STOP, _("Stop path tracer"))) {
				_pathTracer.stop();
			}
			const voxelpathtracer::PathTracerState &state = _pathTracer.state();
			const voxelpathtracer::PathTracerParams &params = state.params;
			ImGui::Text(_("Sample %i / %i"), _currentSample, params.samples);
			_pathTracer.update(&_currentSample);
			_image = _pathTracer.image();
			if (_image->isLoaded()) {
				_texture->upload(_image);
			}
		} else {
			if (ImGui::IconMenuItem(ICON_LC_PLAY, _("Start path tracer"))) {
				_pathTracer.start(sceneGraph, _sceneMgr->activeCamera());
			}
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

void RenderPanel::update(const char *id, const scenegraph::SceneGraph &sceneGraph) {
	core_trace_scoped(RenderPanel);
#if USE_YOCTO
	static ui::ScopedPanel panel(cfg::VoxEditShowRender);
	const core::String &title = makeTitle(ICON_LC_IMAGE, _("Render"), id);
	ui::ScopedPanel::Scope scope =
		panel.begin(title.c_str(), ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_MenuBar |
						ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	_hovered = false;
	if (!scope) {
		_pathTracer.stop();
		return;
	}
	renderMenuBar(sceneGraph);

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

	// TODO: allow to change the current scene camera like in the scene view in the Viewport class
	if (_texture->isLoaded()) {
		ImGui::SetCursorPos(cursorPos);
		ImGui::Image(_texture->handle(), ImVec2((float)_texture->width(), (float)_texture->height()));
	}

	renderCameraManipulator(_camera, headerSize, true);

	_hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem |
									  ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_DockHierarchy);
	if (_hovered) {
		_sceneMgr->setActiveCamera(&_camera, false);
		updateCameraInput(headerSize);
	}
#else
	(void)id;
	(void)sceneGraph;
#endif
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
	if (_texture) {
		_texture->shutdown();
	}
}

} // namespace voxedit
