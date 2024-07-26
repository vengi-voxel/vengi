/**
 * @file
 */

#include "AssetPanel.h"
#include "DragAndDropPayload.h"
#include "ui/IconsLucide.h"
#include "app/Async.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/File.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "ui/IMGUIApp.h"
#include "ui/IMGUIEx.h"
#include "video/Texture.h"
#include "video/gl/GLTypes.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

AssetPanel::AssetPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr,
					   const voxelcollection::CollectionManagerPtr &collectionMgr,
					   const video::TexturePoolPtr &texturePool, const io::FilesystemPtr &filesystem)
	: Super(app, "asset"), _texturePool(texturePool), _filesystem(filesystem), _sceneMgr(sceneMgr), _collectionMgr(collectionMgr),
	  _collectionPanel(app, sceneMgr, collectionMgr, texturePool) {
}

void AssetPanel::shutdown() {
	_future.wait();
	_images.clear();
	_collectionPanel.shutdown();
}

bool AssetPanel::init() {
	if (!_collectionPanel.init()) {
		Log::error("Failed to initialize the collection panel");
		return false;
	}

	_future = app::async([this] () {
		const core::String &dir = _filesystem->specialDir(io::FilesystemDirectories::FS_Dir_Pictures);
		core::DynamicArray<io::FilesystemEntry> entities;
		_filesystem->list(dir, entities);
		for (const auto &e : entities) {
			const core::String &fullName = core::string::path(dir, e.name);
			if (io::isImage(fullName)) {
				_images.emplace(image::loadImage(fullName));
			}
		}
	});

	_collectionMgr->online();
	return true;
}

void AssetPanel::update(const char *id, bool sceneMode, command::CommandExecutionListener &listener) {
	const core::String title = makeTitle(ICON_LC_LIST, _("Assets"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		core_trace_scoped(AssetPanel);
		if (ImGui::BeginTabBar("##assetpaneltabs", ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_FittingPolicyScroll)) {
			if (ImGui::BeginTabItem(_("Models"))) {
				_collectionPanel.update();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(_("Images"))) {
				core_trace_scoped(Images);
				image::ImagePtr image;
				while (_images.pop(image)) {
					if (image->isLoaded()) {
						_texturePool->addImage(image);
					}
				}
				int n = 1;
				ImGuiStyle &style = ImGui::GetStyle();
				const int maxImages = core_max(1, ImGui::GetWindowSize().x / (50 + style.ItemSpacing.x) - 1);
				for (const auto &e : _texturePool->cache()) {
					if (!e->second || !e->second->isLoaded()) {
						continue;
					}
					const video::Id handle = e->second->handle();
					const image::ImagePtr &image = _texturePool->loadImage(e->first);
					core::String id = core::string::format("%i", n - 1);
					ImGui::ImageButton(id.c_str(), handle, ImVec2(50, 50));
					ImGui::TooltipText("%s: %i:%i", image->name().c_str(), image->width(), image->height());
					if (n % maxImages == 0) {
						ImGui::NewLine();
					} else {
						ImGui::SameLine();
					}
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						ImGui::ImageButton(id.c_str(), handle, ImVec2(50, 50));
						ImGui::SetDragDropPayload(dragdrop::ImagePayload, (const void *)&image, sizeof(image),
												  ImGuiCond_Always);
						ImGui::EndDragDropSource();
					}
					++n;
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

} // namespace voxedit
