/**
 * @file
 */

#include "AssetPanel.h"
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

AssetPanel::AssetPanel(const io::FilesystemPtr &filesystem) : _texturePool(filesystem), _filesystem(filesystem) {
	loadTextures(filesystem->specialDir(io::FilesystemDirectories::FS_Dir_Pictures));
}

void AssetPanel::loadTextures(const core::String &dir) {
	core::DynamicArray<io::Filesystem::DirEntry> entities;
	_filesystem->list(dir, entities);
	for (const auto& e : entities) {
		const core::String &fullName = core::string::path(dir, e.name);
		if (io::isImage(fullName)) {
			_texturePool.load(fullName, false, true);
		}
	}
}

void AssetPanel::update(const char *title, command::CommandExecutionListener &listener) {
	if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoDecoration)) {
		core_trace_scoped(AssetPanel);

		if (ImGui::CollapsingHeader("Images", ImGuiTreeNodeFlags_DefaultOpen)) {
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
				ImGui::TooltipText("%s: %i:%i", image->name().c_str(), image->width(), image->height());
				if (n % maxImages == 0) {
					ImGui::NewLine();
				} else {
					ImGui::SameLine();
				}
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
					ImGui::ImageButton(handle, ImVec2(50, 50));
					ImGui::SetDragDropPayload(dragdrop::PlaneImagePayload, (const void *)&image, sizeof(image),
											  ImGuiCond_Always);
					ImGui::EndDragDropSource();
				}
				++n;
			}
		}
	}
	ImGui::End();
}

} // namespace voxedit
