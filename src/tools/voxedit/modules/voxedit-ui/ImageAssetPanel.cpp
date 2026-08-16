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
#include "ui/ScopedPanel.h"
#include "video/Texture.h"
#include "voxedit-util/Config.h"
#include "voxelui/DragAndDropPayload.h"

namespace voxedit {

ImageAssetPanel::ImageAssetPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, const video::TexturePoolPtr &texturePool,
							 const io::FilesystemPtr &filesystem)
	: Super(app, "imageasset"), _texturePool(texturePool), _filesystem(filesystem), _sceneMgr(sceneMgr) {
}

void ImageAssetPanel::shutdown() {
	_images.clear();
	_dragImage = image::ImagePtr();
}

bool ImageAssetPanel::init() {
	app::schedule([this, fs = _filesystem] () {
		const core::String &dir = fs->sysSpecialDir(io::FilesystemDirectories::FS_Dir_Pictures);
		core::DynamicArray<io::FilesystemEntry> entities;
		fs->list(dir, entities);
		for (const auto &e : entities) {
			const core::String &fullName = core::string::path(dir, e.name);
			if (io::isImage(fullName)) {
				_images.emplace(image::loadImageThumbnail(fullName, ThumbnailSize));
			}
		}
	});
	return true;
}

void ImageAssetPanel::update(const char *id) {
	static ui::ScopedPanel panel(cfg::VoxEditShowAssets);
	if (!panel.isOpen()) {
		return;
	}
	const core::String &title = makeTitle(ICON_LC_LIST, _("Images"), id);
	if (ui::ScopedPanel::Scope scope = panel.begin(title.c_str(), ImGuiWindowFlags_NoFocusOnAppearing)) {
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
			ImGui::TooltipText("%s", image->name().c_str());
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				// Reload full-resolution image for drop targets (fill plane, texture brush, palette)
				if (!_dragImage || _dragImage->name() != image->name()) {
					_dragImage = image::loadImage(image->name());
				}
				// Fall back to the pooled image (thumbnail) if the full reload failed
				if (!_dragImage || !_dragImage->isLoaded()) {
					_dragImage = image;
				}
				ImGui::ImageButton(imgId.c_str(), handle, ImVec2(imageSize, imageSize));
				voxelui::dragdrop::setImagePayload(_dragImage);
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
}

} // namespace voxedit
