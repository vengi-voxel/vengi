/**
 * @file
 */

#include "RenderPanel.h"
#include "IMGUIApp.h"
#include "core/SharedPtr.h"
#include "imgui.h"
#include "scenegraph/SceneGraph.h"
#include "ui/IMGUIEx.h"
#include "video/Texture.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxelpathtracer/PathTracer.h"

namespace voxedit {

bool RenderPanel::init() {
	_texture = video::createEmptyTexture("pathtracer");

	// default params
	yocto::trace_params &params = _pathTracer.state().params;
	params.samples = 16;
	params.envhidden = true;
	params.resolution = 512;

	return true;
}

void RenderPanel::renderMenuBar(const scenegraph::SceneGraph &sceneGraph) {
	if (ImGui::BeginMenuBar()) {
		if (_image && _image->isLoaded()) {
			if (ImGui::Button(_("Save image"))) {
				_app->saveDialog([=](const core::String &file,
									 const io::FormatDescription *desc) { image::writeImage(_image, file); },
								 {}, io::format::images(), "render.png");
			}
		}
		ImGui::Dummy(ImVec2(20, 0));
		if (_pathTracer.started()) {
			if (ImGui::Button(_("Stop path tracer"))) {
				_pathTracer.stop();
			}
			const voxelpathtracer::PathTracerState &state = _pathTracer.state();
			const yocto::trace_params &params = state.params;
			ImGui::TooltipText(_("Sample %i / %i"), _currentSample, params.samples);
			_pathTracer.update(&_currentSample);
			_image = _pathTracer.image();
			if (_image->isLoaded()) {
				_texture->upload(_image);
			}
		} else {
			if (ImGui::Button(_("Start path tracer"))) {
				_pathTracer.start(sceneGraph, _sceneMgr->activeCamera());
			}
		}
		ImGui::EndMenuBar();
	}
}

void RenderPanel::update(const char *title, const scenegraph::SceneGraph &sceneGraph) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_MenuBar)) {
		if (_pathTracer.started()) {
			voxelpathtracer::PathTracerState &state = _pathTracer.state();
			yocto::trace_params &params = state.params;
			if (params.resolution != ImGui::GetContentRegionAvail().x) {
				params.resolution = ImGui::GetContentRegionAvail().x;
				_pathTracer.restart(sceneGraph, _sceneMgr->activeCamera());
			}
		}
		renderMenuBar(sceneGraph);
		// TODO: allow to change the current scene camera like in the scene view in the Viewport class
		if (_texture->isLoaded()) {
			ImGui::Image(_texture->handle(), ImVec2((float)_texture->width(), (float)_texture->height()));
		}
	}
	ImGui::End();
}

void RenderPanel::updateSettings(const char *title, const scenegraph::SceneGraph &sceneGraph) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		voxelpathtracer::PathTracerState &state = _pathTracer.state();
		yocto::trace_params &params = state.params;
		int changed = 0;
		changed += ImGui::InputInt(_("Dimensions"), &params.resolution, 180, 4096, ImGuiInputTextFlags_ReadOnly);
		changed += ImGui::InputInt(_("Samples"), &params.samples, 16, 4096);
		changed += ImGui::ComboItems(_("Tracer"), (int *)&params.sampler, yocto::trace_sampler_names);
		changed += ImGui::ComboItems(_("False color"), (int *)&params.falsecolor, yocto::trace_falsecolor_names);
		changed += ImGui::SliderInt(_("Bounces"), &params.bounces, 1, 128);
		changed += ImGui::SliderInt(_("Batch"), &params.batch, 1, 16);
		changed += ImGui::SliderFloat(_("Clamp"), &params.clamp, 10, 1000);
		changed += ImGui::SliderInt(_("Preview ratio"), &params.pratio, 1, 64);
		changed += ImGui::Checkbox(_("Hide environment"), &params.envhidden);
		changed += ImGui::Checkbox(_("Filter"), &params.tentfilter);
		changed += ImGui::Checkbox(_("Denoise"), &params.denoise);

		if (!state.scene.camera_names.empty()) {
			changed += ImGui::ComboItems(_("Camera"), &params.camera, state.scene.camera_names);
		}
		if (changed > 0) {
			if (const video::Camera *camera = _sceneMgr->activeCamera()) {
				_pathTracer.restart(sceneGraph, camera);
			}
		}
	}
	ImGui::End();
}

void RenderPanel::shutdown() {
	if (_texture) {
		_texture->shutdown();
	}
}

} // namespace voxedit
