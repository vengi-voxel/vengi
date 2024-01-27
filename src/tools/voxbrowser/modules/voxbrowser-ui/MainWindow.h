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
	voxelformat::ThumbnailContext _thumbnailCtx;
	video::TexturePool _texturePool;

	void configureLeftTopWidgetDock(ImGuiID dockId);
	void configureMainTopWidgetDock(ImGuiID dockId);
	void configureMainBottomWidgetDock(ImGuiID dockId);

	bool createThumbnailIfNeeded(VoxelFile &voxelFile);
	bool downloadThumbnailIfNeeded(VoxelFile &voxelFile);
	bool downloadIfNeeded(const VoxelFile &voxelFile);

	void updateAssetList(const voxbrowser::VoxelFileMap &voxelFilesMap);
	void updateAsset();
	void updateAssetDetails();

	bool filtered(const VoxelFile &voxelFile) const;
	void updateFilters();

public:
	MainWindow(ui::IMGUIApp *app);
	virtual ~MainWindow();
	bool init();
	void update(const voxbrowser::VoxelFileMap &voxelFiles);
	void shutdown();
};

} // namespace voxbrowser
