/**
 * @file
 */

#include "ToolsPanel.h"
#include "DragAndDropPayload.h"
#include "IMGUIApp.h"
#include "Util.h"
#include "image/Image.h"
#include "imgui.h"
#include "io/File.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "ui/imgui/IMGUIEx.h"
#include "ui/imgui/IconsFontAwesome5.h"
#include "ui/imgui/IconsForkAwesome.h"
#include "video/Texture.h"
#include "video/gl/GLTypes.h"
#include "voxedit-util/SceneManager.h"
#include "voxelutil/VoxelUtil.h"

namespace voxedit {

ToolsPanel::ToolsPanel(const io::FilesystemPtr &filesystem) : _texturePool(filesystem), _filesystem(filesystem) {
	loadTextures(filesystem->specialDir(io::FilesystemDirectories::FS_Dir_Pictures));
}

void ToolsPanel::loadTextures(const core::String &dir) {
	core::DynamicArray<io::Filesystem::DirEntry> entities;
	_filesystem->list(dir, entities);
	for (const auto& e : entities) {
		const core::String &fullName = core::string::path(dir, e.name);
		if (io::isImage(fullName)) {
			_texturePool.load(fullName, false, true);
		}
	}
}

bool ToolsPanel::mirrorAxisRadioButton(const char *title, math::Axis type) {
	voxedit::ModifierFacade &modifier = sceneMgr().modifier();
	ui::imgui::ScopedStyle style;
	veui::AxisStyleText(style, type, false);
	if (ImGui::RadioButton(title, modifier.mirrorAxis() == type)) {
		modifier.setMirrorAxis(type, sceneMgr().referencePosition());
		return true;
	}
	return false;
}

void ToolsPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		core_trace_scoped(ModifierPanel);

		if (ImGui::CollapsingHeader("Action", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::CommandButton(ICON_FA_CROP, "crop", nullptr, 0, &listener);
			ImGui::SameLine();
			ImGui::CommandButton(ICON_FA_EXPAND_ARROWS_ALT, "resize", nullptr, 0, &listener);
			ImGui::SameLine();
			ImGui::CommandButton(ICON_FA_OBJECT_UNGROUP, "colortolayer", nullptr, 0, &listener);
			ImGui::SameLine();
			ImGui::CommandButton(ICON_FA_COMPRESS_ALT, "scale", nullptr, 0, &listener);
			ImGui::SameLine();
			ImGui::CommandButton(ICON_FA_FILL_DRIP, "fillhollow", nullptr, 0, &listener);
		}

		ImGui::NewLine();

		const float buttonWidth = (float)imguiApp()->fontSize() * 4;
		if (ImGui::CollapsingHeader("Rotate on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			veui::AxisButton(math::Axis::X, "X##rotate", "rotate 90 0 0", ICON_FK_REPEAT, nullptr, buttonWidth,
							 &listener);
			ImGui::TooltipText("Rotate by 90 degree on the x axis");
			ImGui::SameLine();
			veui::AxisButton(math::Axis::Y, "Y##rotate", "rotate 0 90 0", ICON_FK_REPEAT, nullptr, buttonWidth,
							 &listener);
			ImGui::TooltipText("Rotate by 90 degree on the y axis");
			ImGui::SameLine();
			veui::AxisButton(math::Axis::Z, "Z##rotate", "rotate 0 0 90", ICON_FK_REPEAT, nullptr, buttonWidth,
							 &listener);
			ImGui::TooltipText("Rotate by 90 degree on the z axis");
		}

		ImGui::NewLine();

		if (ImGui::CollapsingHeader("Flip on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			veui::AxisButton(math::Axis::X, "X##flip", "flip x", nullptr, nullptr, buttonWidth, &listener);
			ImGui::SameLine();
			veui::AxisButton(math::Axis::Y, "Y##flip", "flip y", nullptr, nullptr, buttonWidth, &listener);
			ImGui::SameLine();
			veui::AxisButton(math::Axis::Z, "Z##flip", "flip z", nullptr, nullptr, buttonWidth, &listener);
		}

		ImGui::NewLine();

		if (ImGui::CollapsingHeader("Mirror on axis", ImGuiTreeNodeFlags_DefaultOpen)) {
			mirrorAxisRadioButton("None##mirror", math::Axis::None);
			ImGui::SameLine();
			mirrorAxisRadioButton("X##mirror", math::Axis::X);
			ImGui::SameLine();
			mirrorAxisRadioButton("Y##mirror", math::Axis::Y);
			ImGui::SameLine();
			mirrorAxisRadioButton("Z##mirror", math::Axis::Z);
		}

		if (ImGui::CollapsingHeader("Plane", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (ImGui::Button(ICON_FK_FILE_IMAGE_O " Open image directory")) {
				auto callback = [this](const core::String &dir) {
					loadTextures(dir);
				};
				imguiApp()->fileDialog(callback, video::WindowedApp::OpenFileMode::Directory);
			}
			int n = 1;
			ImGuiStyle& style = ImGui::GetStyle();
			const int maxImages = core_max(1, ImGui::GetWindowSize().x / (50 + style.ItemSpacing.x) - 1);
			for (const auto &e : _texturePool.cache()) {
				if (!e->second || !e->second->isLoaded()) {
					continue;
				}
				const video::Id handle = e->second->handle();
				const image::ImagePtr &image = _texturePool.loadImage(e->first, false);
				ImGui::ImageButton(handle, ImVec2(50, 50));
				if (n % maxImages == 0) {
					ImGui::NewLine();
				} else {
					ImGui::SameLine();
				}
				ImGui::TooltipText("%s: %i:%i", image->name().c_str(), image->width(), image->height());
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
					ImGui::ImageButton(handle, ImVec2(50, 50));
					ImGui::SetDragDropPayload(dragdrop::PlaneImagePayload, (const void *)&image, sizeof(image),
											  ImGuiCond_Always);
					ImGui::EndDragDropSource();
				}
				++n;
			}
			ImGui::NewLine();
		}
	}
	ImGui::End();
}

} // namespace voxedit
