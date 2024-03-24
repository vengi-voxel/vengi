/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "ui/IMGUIEx.h"
#include "video/TexturePool.h"
#include "voxbrowser-ui/MenuBar.h"
#include "voxbrowser-ui/StatusBar.h"
#include "voxelcollection/Downloader.h"
#include "voxelformat/FormatThumbnail.h"

namespace voxbrowser {

class MainWindow : public ui::Panel {
private:
	using Super = ui::Panel;

	core::DynamicArray<io::FormatDescription> _filterEntries;

	float _filterFormatTextWidth = -1.0f;
	int _currentFilterFormatEntry = -1;
	core::String _currentFilterName;
	core::String _currentFilterLicense;

	StatusBar _statusBar;
	MenuBar _menuBar;
	voxelcollection::VoxelFile _selected;
	video::TexturePool &_texturePool;

	struct ThumbnailProperties {
		ImVec2 translate{0.0f, 0.0f};
		ImVec2 scale{1.0f, 1.0f};
	};
	ThumbnailProperties _thumbnailProperties;
	voxelformat::ThumbnailContext _thumbnailCtx;

	void image(const video::TexturePtr &texture);

	void createThumbnail(const voxelcollection::VoxelFile &voxelFile);
	video::TexturePtr thumbnailLookup(const voxelcollection::VoxelFile &voxelFile);

	void configureLeftTopWidgetDock(ImGuiID dockId);
	void configureMainTopWidgetDock(ImGuiID dockId);
	void configureMainBottomWidgetDock(ImGuiID dockId);

	int buildVoxelTree(const voxelcollection::VoxelFiles &voxelFiles);
	int updateAssetList(const voxelcollection::VoxelFileMap &voxelFilesMap);
	void updateAsset();
	void updateAssetDetails();

	bool filtered(const voxelcollection::VoxelFile &voxelFile) const;
	void updateFilters();
	bool isFilterActive() const;

	void popupAbout();
	void registerPopups();

public:
	MainWindow(ui::IMGUIApp *app, video::TexturePool &texturePool);
	virtual ~MainWindow();
	bool init();
	void update(const voxelcollection::VoxelFileMap &voxelFiles, int downloadProgress, int allEntries);
	void shutdown();
};

} // namespace voxbrowser
