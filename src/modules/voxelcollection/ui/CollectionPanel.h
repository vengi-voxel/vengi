/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "video/TexturePool.h"
#include "voxelcollection/Downloader.h"

#define TITLE_ASSET_LIST "Assets##list"

namespace voxelcollection {

class CollectionPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	core::DynamicArray<io::FormatDescription> _filterEntries;

	float _filterFormatTextWidth = -1.0f;
	int _currentFilterFormatEntry = -1;
	bool _newSelected = false;
	core::String _currentFilterName;
	core::String _currentFilterLicense;
	voxelcollection::VoxelFile _selected;
	video::TexturePoolPtr _texturePool;

	int buildVoxelTree(const voxelcollection::VoxelFiles &voxelFiles, const std::function<void(VoxelFile &voxelFile)> &contextMenu);

	bool filtered(const voxelcollection::VoxelFile &voxelFile) const;
	void updateFilters();
	bool isFilterActive() const;

public:
	CollectionPanel(ui::IMGUIApp *app, const video::TexturePoolPtr &texturePool);
	virtual ~CollectionPanel();

	bool init();
	int update(const voxelcollection::VoxelFileMap &voxelFilesMap, const std::function<void(VoxelFile &voxelFile)> &contextMenu = {});
	void shutdown();

	bool newSelected() const;

	video::TexturePtr thumbnailLookup(const voxelcollection::VoxelFile &voxelFile);
	voxelcollection::VoxelFile &selected();
};

inline bool CollectionPanel::newSelected() const {
	return _newSelected;
}

} // namespace voxelcollection
