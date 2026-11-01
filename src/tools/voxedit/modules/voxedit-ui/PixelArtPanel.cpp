/**
 * @file
 */

#include "PixelArtPanel.h"
#include "app/I18N.h"
#include "core/ArrayLength.h"
#include "core/Common.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/FormatDescription.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphImage.h"
#include "ui/ScopedPanel.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/Face.h"
#include "voxelutil/ImageUtils.h"
#include <glm/common.hpp>
#include <string.h>

namespace voxedit {

PixelArtPanel::~PixelArtPanel() {
	if (_texture) {
		_texture->shutdown();
		_texture = {};
	}
}

static void mixMatrix(uint64_t &hash, const glm::mat4 &matrix) {
	const float *values = &matrix[0][0];
	for (int i = 0; i < 16; ++i) {
		uint32_t bits = 0;
		memcpy(&bits, &values[i], sizeof(bits));
		hash = (hash * 16777619u) ^ bits;
	}
}

image::ImagePtr PixelArtPanel::pixelArtImage() {
	const bool isometric = core::getVar(cfg::ImageIsometric)->boolVal();
	if (isometric && _view > 3) {
		_view = 0;
	}
	const voxel::FaceNames isoFaces[] = {voxel::FaceNames::Front, voxel::FaceNames::Right, voxel::FaceNames::Back,
										 voxel::FaceNames::Left};
	const voxel::FaceNames flatFaces[] = {voxel::FaceNames::Front, voxel::FaceNames::Right, voxel::FaceNames::Back,
										  voxel::FaceNames::Left, voxel::FaceNames::Up, voxel::FaceNames::Down};
	const voxel::FaceNames frontFace = isometric ? isoFaces[_view] : flatFaces[_view];
	voxelutil::IsoRenderConfig isoConfig = voxelutil::IsoRenderConfig::fromCvars();
	isoConfig.frontFace = frontFace;
	voxelutil::ImageRenderConfig imageConfig = voxelutil::ImageRenderConfig::fromCvars();
	imageConfig.frontFace = frontFace;
	const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
	const scenegraph::FrameIndex frameIdx = _sceneMgr->currentFrame();
	// The stamp loop walks the combined scene bounds, so a sparse but huge scene is refused.
	if (isometric && _sceneMgr->exceedsMaxSuggestedVolumeSize(sceneGraph.sceneRegion(frameIdx, true))) {
		_image = {};
		_hash = 0;
		_configHash = 0;
		return {};
	}
	uint64_t transformHash = (uint64_t)frameIdx;
	sceneGraph.visitChildren(sceneGraph.root().id(), true, [&](const scenegraph::SceneGraphNode &node) {
		if (!node.isAnyModelNode() || !node.visible()) {
			return;
		}
		mixMatrix(transformHash, sceneGraph.transformForFrame(node, frameIdx).worldMatrix());
	});
	const uint64_t configHash = isometric ? isoConfig.hash() : (imageConfig.hash() ^ 0x3244000000000000ull);
	const uint64_t hash = _sceneMgr->calculateHash() ^ configHash ^ transformHash;
	if (!_image || hash != _hash || configHash != _configHash) {
		if (isometric) {
			_image = scenegraph::renderIsometricImage(sceneGraph, isoConfig, InvalidNodeId, frameIdx);
		} else {
			_image = scenegraph::renderToImage(sceneGraph, imageConfig, InvalidNodeId, frameIdx);
		}
		_hash = hash;
		_configHash = configHash;
		if (_image) {
			if (_texture && (_texture->width() != _image->width() || _texture->height() != _image->height())) {
				_texture->shutdown();
				_texture = {};
			}
			if (!_texture) {
				video::TextureConfig texConfig = video::createDefaultTextureConfig();
				texConfig.filter(video::TextureFilter::Nearest);
				_texture = core::make_shared<video::Texture>(texConfig, _image->width(), _image->height(), "pixelart");
			}
			_texture->upload(_image);
		}
	}
	return _image;
}

core::String PixelArtPanel::suggestedSaveName() const {
	const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNodeByUUID(_sceneMgr->activeNodeUUID());
	core::String name;
	if (node != nullptr) {
		name = core::string::sanitizeFilename(node->name());
	}
	if (name.empty()) {
		name = "pixelart";
	}
	return name + "." + io::format::png().mainExtension(false);
}

void PixelArtPanel::saveImage() {
	if (!_image || !_image->isLoaded()) {
		return;
	}
	const image::ImagePtr image = _image;
	const core::String filename = suggestedSaveName();
	_app->saveDialog(
		[image](const core::String &file, const io::FormatDescription *desc) {
			core::String outfile = file;
			if (desc != nullptr && !desc->exts.empty() &&
				!desc->matchesExtension(core::string::extractExtension(outfile))) {
				outfile = core::string::replaceExtension(outfile, desc->mainExtension());
			}
			if (!image::writePNG(image, outfile)) {
				Log::error("Failed to save pixel art image %s", outfile.c_str());
				return;
			}
			Log::info("Saved pixel art image %s", outfile.c_str());
		},
		{}, io::format::images(), filename);
}

void PixelArtPanel::update(const char *id) {
	core_trace_scoped(PixelArtPanel);
	static ui::ScopedPanel panel(cfg::VoxEditShowPixelArt);
	if (!panel.isOpen()) {
		return;
	}
	const core::String &title = makeTitle(ICON_LC_CUBOID, _("Pixel art"), id);
	if (ui::ScopedPanel::Scope scope =
			panel.begin(title.c_str(), ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_MenuBar)) {
		const image::ImagePtr img = pixelArtImage();
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginIconMenu(ICON_LC_FILE, _("File"))) {
				if (ImGui::IconMenuItem(ICON_LC_SAVE, _("Save"), nullptr, false, img && img->isLoaded())) {
					saveImage();
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		const bool isometric = core::getVar(cfg::ImageIsometric)->boolVal();
		const char *modeLabel = isometric ? _("Isometric") : _("2D");
		if (ImGui::BeginCombo(_("Mode###pixelartmode"), modeLabel)) {
			if (ImGui::Selectable(_("Isometric"), isometric)) {
				core::getVar(cfg::ImageIsometric)->setVal(true);
			}
			if (isometric) {
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::Selectable(_("2D"), !isometric)) {
				core::getVar(cfg::ImageIsometric)->setVal(false);
			}
			if (!isometric) {
				ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		const char *isoViews[] = {_("Front"), _("Right"), _("Back"), _("Left")};
		const char *flatViews[] = {_("Front"), _("Right"), _("Back"), _("Left"), _("Top"), _("Bottom")};
		const char **views = isometric ? isoViews : flatViews;
		const int viewCount = isometric ? lengthof(isoViews) : lengthof(flatViews);
		if (ImGui::BeginCombo(_("View###pixelartview"), views[_view])) {
			for (int i = 0; i < viewCount; ++i) {
				if (ImGui::Selectable(views[i], _view == i)) {
					_view = i;
				}
				if (_view == i) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::ColorEdit4Var(cfg::ImageBackground);
		ImGui::InputVarInt(cfg::ImageWidth);
		ImGui::InputVarInt(cfg::ImageHeight);
		ImGui::CheckboxVar(cfg::ImageUpScale);
		if (isometric) {
			ImGui::CheckboxVar(cfg::ImageIsoShading);
			ImGui::CheckboxVar(cfg::ImageIsoMaterials);
			ImGui::CheckboxVar(cfg::ImageIsoTransparency);
			ImGui::CheckboxVar(cfg::ImageIsoOutline);
			ImGui::ColorEdit4Var(cfg::ImageIsoOutlineColor);
		} else {
			ImGui::SliderVarFloat(cfg::ImageDepthFactor2D);
		}

		if (isometric && _sceneMgr->exceedsMaxSuggestedVolumeSize(
							 _sceneMgr->sceneGraph().sceneRegion(_sceneMgr->currentFrame(), true))) {
			ImGui::TextWrapped(
				_("The scene is larger than the suggested volume size. Isometric preview is disabled."));
			return;
		}

		if (!img || !_texture) {
			ImGui::TextUnformatted(_("Select a voxel model to preview."));
			return;
		}

		if (img->width() <= 0 || img->height() <= 0) {
			ImGui::TextUnformatted(_("Select a voxel model to preview."));
			return;
		}
		const ImVec2 available = ImGui::GetContentRegionAvail();
		const float dpi = core_max(1.0f, ImGui::GetWindowDpiScale());
		const int fit = core_max(1, (int)glm::floor(core_min(available.x * dpi / (float)img->width(),
															 available.y * dpi / (float)img->height())));
		if (!_zoomUser || _zoom < 1) {
			_zoom = fit;
		}

		// The child clips the enlarged image. Wheel zooms instead of scrolling.
		ImGui::BeginChild("##pixelartscroll", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
						  ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		const float scrollX = glm::floor(ImGui::GetScrollX() * dpi + 0.5f) / dpi;
		const float scrollY = glm::floor(ImGui::GetScrollY() * dpi + 0.5f) / dpi;
		if (scrollX != ImGui::GetScrollX()) {
			ImGui::SetScrollX(scrollX);
		}
		if (scrollY != ImGui::GetScrollY()) {
			ImGui::SetScrollY(scrollY);
		}

		const float texel = (float)_zoom / dpi;
		const ImVec2 size((float)img->width() * texel, (float)img->height() * texel);
		const ImVec2 cursor = ImGui::GetCursorScreenPos();
		ImGui::SetCursorScreenPos(ImVec2(glm::floor(cursor.x * dpi) / dpi, glm::floor(cursor.y * dpi) / dpi));
		ImGui::InvisibleButton("##pixelartcanvas", size);
		const ImVec2 origin = ImGui::GetItemRectMin();
		const ImVec2 drawn(origin.x + size.x, origin.y + size.y);

		ImDrawList *drawList = ImGui::GetWindowDrawList();
		const ImGuiPlatformIO &platform = ImGui::GetPlatformIO();
		// ImGui binds a linear sampler over the texture. Point sampling keeps every texel a hard square.
		if (platform.DrawCallback_SetSamplerNearest != nullptr) {
			drawList->AddCallback(platform.DrawCallback_SetSamplerNearest, nullptr);
		}
		drawList->AddImage(_texture->handle(), origin, drawn);
		if (platform.DrawCallback_SetSamplerLinear != nullptr) {
			drawList->AddCallback(platform.DrawCallback_SetSamplerLinear, nullptr);
		}

		ImGuiIO &io = ImGui::GetIO();
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			ImGui::SetScrollX(ImGui::GetScrollX() - io.MouseDelta.x);
			ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);
		}
		if (ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY) && io.MouseWheel != 0.0f) {
			_zoomWheel += io.MouseWheel;
			const int steps = (int)_zoomWheel;
			_zoomWheel -= (float)steps;
			if (steps != 0) {
				const int zoomMax = core_max(64, fit * 4);
				const int newZoom = core_max(1, core_min(zoomMax, _zoom + steps));
				if (newZoom != _zoom) {
					const ImVec2 mouse = io.MousePos;
					const float u = (mouse.x - origin.x) / texel;
					const float v = (mouse.y - origin.y) / texel;
					const ImVec2 winPos = ImGui::GetWindowPos();
					const float contentX = ImGui::GetScrollX() + (origin.x - winPos.x);
					const float contentY = ImGui::GetScrollY() + (origin.y - winPos.y);
					const float newTexel = (float)newZoom / dpi;
					ImGui::SetScrollX(contentX + u * newTexel - (mouse.x - winPos.x));
					ImGui::SetScrollY(contentY + v * newTexel - (mouse.y - winPos.y));
					_zoom = newZoom;
					_zoomUser = true;
				}
			}
		}
		ImGui::EndChild();
	}
}

} // namespace voxedit
