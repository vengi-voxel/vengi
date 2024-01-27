/**
 * @file
 */

#pragma once

#include "ui/IMGUIApp.h"
#include "video/TexturePool.h"
#include "voxbrowser-ui/MenuBar.h"
#include "voxbrowser-ui/StatusBar.h"
#include "voxbrowser-util/Downloader.h"
#include "voxelformat/FormatThumbnail.h"

namespace voxbrowser {

class MainWindow {
private:
	ui::IMGUIApp *_app;
	core::DynamicArray<io::FormatDescription> _filterEntries;
	float _filterTextWidth = -1.0f;
	int _currentFilterEntry = -1;
	StatusBar _statusBar;
	MenuBar _menuBar;
	VoxelFile _selected;
	video::TexturePool &_texturePool;

	struct ThumbnailProperties {
		ImVec2 translate{0.0f, 0.0f};
		ImVec2 scale{1.0f, 1.0f};
	};
	ThumbnailProperties _thumbnailProperties;
	voxelformat::ThumbnailContext _thumbnailCtx;

	void image(const video::TexturePtr &texture);

	void createThumbnail(const VoxelFile &voxelFile);
	video::TexturePtr thumbnailLookup(const VoxelFile &voxelFile);

	void configureLeftTopWidgetDock(ImGuiID dockId);
	void configureMainTopWidgetDock(ImGuiID dockId);
	void configureMainBottomWidgetDock(ImGuiID dockId);

	void updateAssetList(const VoxelFileMap &voxelFilesMap);
	void updateAsset();
	void updateAssetDetails();

	bool filtered(const VoxelFile &voxelFile) const;
	void updateFilters();

public:
	MainWindow(ui::IMGUIApp *app, video::TexturePool &texturePool);
	virtual ~MainWindow();
	bool init();
	void update(const VoxelFileMap &voxelFiles);
	void shutdown();
};

} // namespace voxbrowser
