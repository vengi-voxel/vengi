/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "ui/IMGUIEx.h"
#include "video/TexturePool.h"
#include "voxbrowser-ui/MenuBar.h"
#include "voxbrowser-ui/StatusBar.h"
#include "voxelcollection/CollectionManager.h"
#include "voxelcollection/ui/CollectionPanel.h"
#include "voxelcollection/Downloader.h"
#include "voxelformat/FormatThumbnail.h"

namespace voxbrowser {

class MainWindow : public ui::Panel {
private:
	using Super = ui::Panel;

	voxelcollection::CollectionPanel _voxelCollection;
	StatusBar _statusBar;
	MenuBar _menuBar;
	video::TexturePoolPtr _texturePool;

	struct ThumbnailProperties {
		ImVec2 translate{0.0f, 0.0f};
		ImVec2 scale{1.0f, 1.0f};
	};
	ThumbnailProperties _thumbnailProperties;
	voxelformat::ThumbnailContext _thumbnailCtx;

	void image(const video::TexturePtr &texture);

	void createThumbnail(const voxelcollection::VoxelFile &voxelFile);

	void configureLeftTopWidgetDock(ImGuiID dockId);
	void configureMainTopWidgetDock(ImGuiID dockId);
	void configureMainBottomWidgetDock(ImGuiID dockId);

	void updateAsset();
	void updateAssetDetails();

	void popupAbout();
	void registerPopups();

public:
	MainWindow(ui::IMGUIApp *app, const video::TexturePoolPtr &texturePool);
	virtual ~MainWindow();
	bool init();
	void update(voxelcollection::CollectionManager &collectionMgr);
	void shutdown();
};

} // namespace voxbrowser
