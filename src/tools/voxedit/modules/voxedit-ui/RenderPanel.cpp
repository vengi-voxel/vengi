/**
 * @file
 */

#include "RenderPanel.h"
#include "core/SharedPtr.h"
#include "io/FileStream.h"
#include "scenegraph/SceneGraph.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "video/Texture.h"
#include "voxedit-util/SceneManager.h"
#include "voxelpathtracer/PathTracer.h"
#include "voxelpathtracer/PathTracerState.h"

namespace voxedit {

bool RenderPanel::init() {
	_texture = video::createEmptyTexture("pathtracer");
	return true;
}

void RenderPanel::renderMenuBar(const scenegraph::SceneGraph &sceneGraph) {
	if (ImGui::BeginMenuBar()) {
		if (_image && _image->isLoaded()) {
			if (ImGui::Button(_("Save image"))) {
				_app->saveDialog(
					[=](const core::String &file, const io::FormatDescription *desc) {
						const io::FilePtr &filePtr = _app->filesystem()->open(file, io::FileMode::SysWrite);
						io::FileStream stream(filePtr);
						image::writePNG(_image, stream);
					},
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

void RenderPanel::update(const char *id, const scenegraph::SceneGraph &sceneGraph) {
	core_trace_scoped(RenderPanel);
	const core::String title = makeTitle(ICON_LC_IMAGE, _("Render"), id);
	if (ImGui::Begin(title.c_str(), nullptr,
					 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_MenuBar)) {
		renderMenuBar(sceneGraph);
		// TODO: allow to change the current scene camera like in the scene view in the Viewport class
		if (_texture->isLoaded()) {
			ImGui::Image(_texture->handle(), ImVec2((float)_texture->width(), (float)_texture->height()));
		}
	} else {
		_pathTracer.stop();
	}
	ImGui::End();
}

void RenderPanel::updateSettings(const char *id, const scenegraph::SceneGraph &sceneGraph) {
	const core::String title = makeTitle(ICON_LC_IMAGE, _("Render Settings"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		voxelpathtracer::PathTracerState &state = _pathTracer.state();
		yocto::trace_params &params = state.params;
		int changed = 0;
		changed += ImGui::InputInt(_("Dimensions"), &params.resolution);
		changed += ImGui::ComboItems(_("Tracer"), (int *)&params.sampler, yocto::trace_sampler_names);
		changed += ImGui::InputInt(_("Samples"), &params.samples, 16, 4096);
		ImGui::TooltipTextUnformatted(_("The number of per-pixel samples used while rendering and is the only "
										"parameter used to control the tradeoff between noise and speed."));
		changed += ImGui::SliderInt(_("Bounces"), &params.bounces, 1, 128);
		ImGui::TooltipTextUnformatted(_("The maximum number of bounces and should be high for scenes with glass and "
										"volumes, but otherwise a low number would suffice."));
		changed += ImGui::SliderFloat(_("Clamp"), &params.clamp, 10, 1000);
		ImGui::TooltipTextUnformatted(_("Remove high-energy fireflies"));
		changed += ImGui::SliderInt(_("Preview ratio"), &params.pratio, 1, 64);
		changed += ImGui::SliderInt(_("Batch"), &params.batch, 1, 16);

		changed += ImGui::Checkbox(_("No caustics"), &params.nocaustics);
		ImGui::TooltipTextUnformatted(_("Removes certain path that cause caustics"));
		changed += ImGui::Checkbox(_("Hide environment"), &params.envhidden);
		ImGui::TooltipTextUnformatted(_("Removes the environment map from the camera rays."));
		changed += ImGui::Checkbox(_("Filter"), &params.tentfilter);
		ImGui::TooltipTextUnformatted(_("Apply a linear filter to the image pixels"));
		changed += ImGui::Checkbox(_("High Quality BVH"), &params.highqualitybvh);
		ImGui::TooltipTextUnformatted(_("High quality bounding volume hierarchy"));
		changed += ImGui::Checkbox(_("Denoise"), &params.denoise);

		if (ImGui::Button(_("Reset all"))) {
			params = yocto::trace_params();
			++changed;
		}
		if (ImGui::Button(_("High quality"))) {
			params = yocto::trace_params();
			params.sampler = yocto::trace_sampler_type::path; // path tracing
			params.samples = 1024;							  // high-sample count
			params.bounces = 64;							  // high max bounces
			++changed;
		}
		if (ImGui::Button(_("Geometry preview"))) {
			params = yocto::trace_params();
			params.sampler = yocto::trace_sampler_type::eyelight; // geometry preview
			params.samples = 16;								  // low-sample count
			++changed;
		}

		if (!state.scene.camera_names.empty()) {
			changed += ImGui::ComboItems(_("Camera"), &params.camera, state.scene.camera_names);
		}
		if (changed > 0) {
			_pathTracer.restart(sceneGraph, _sceneMgr->activeCamera());
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
