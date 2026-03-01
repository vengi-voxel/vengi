/**
 * @file
 */

#include "ImageAssetPanel.h"
#include "app/Async.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/File.h"
#include "io/FormatDescription.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "video/Texture.h"
#include "voxelui/DragAndDropPayload.h"

namespace voxedit {

ImageAssetPanel::ImageAssetPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, const video::TexturePoolPtr &texturePool,
							 const io::FilesystemPtr &filesystem)
	: Super(app, "imageasset"), _texturePool(texturePool), _filesystem(filesystem), _sceneMgr(sceneMgr) {
}

void ImageAssetPanel::shutdown() {
	_images.clear();
}

bool ImageAssetPanel::init() {
	app::schedule([this, fs = _filesystem] () {
		const core::String &dir = fs->sysSpecialDir(io::FilesystemDirectories::FS_Dir_Pictures);
		core::DynamicArray<io::FilesystemEntry> entities;
		fs->list(dir, entities);
		for (const auto &e : entities) {
			const core::String &fullName = core::string::path(dir, e.name);
			if (io::isImage(fullName)) {
				_images.emplace(image::loadImage(fullName));
			}
		}
	});
	return true;
}

void ImageAssetPanel::update(const char *id) {
	const core::String title = makeTitle(ICON_LC_LIST, _("Images"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		core_trace_scoped(ImageAssetPanel);
		image::ImagePtr loadImage;
		while (_images.pop(loadImage)) {
			if (loadImage->isLoaded()) {
				_texturePool->addImage(loadImage);
			}
		}
		int n = 1;
		ImGuiStyle &style = ImGui::GetStyle();
		const float imageSize = 50.0f * style.FontScaleDpi;
		const int maxImages = core_max(1, ImGui::GetWindowSize().x / (imageSize + style.ItemSpacing.x) - 1);
		for (const auto &e : _texturePool->cache()) {
			if (!e->second || !e->second->isLoaded()) {
				continue;
			}
			const video::Id handle = e->second->handle();
			const image::ImagePtr &image = _texturePool->loadImage(e->first);
			core::String imgId = core::String::format("##image-%i", n - 1);
			ImGui::ImageButton(imgId.c_str(), handle, ImVec2(imageSize, imageSize));
			ImGui::TooltipText("%s: %i:%i", image->name().c_str(), image->width(), image->height());
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				ImGui::ImageButton(imgId.c_str(), handle, ImVec2(imageSize, imageSize));
				ImGui::SetDragDropPayload(voxelui::dragdrop::ImagePayload, (const void *)&image, sizeof(image), ImGuiCond_Always);
				ImGui::EndDragDropSource();
			}
			if (n % maxImages == 0) {
				ImGui::NewLine();
			} else {
				ImGui::SameLine();
			}
			++n;
		}
	}
	ImGui::End();
}

} // namespace voxedit
