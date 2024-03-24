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
	core::String _currentFilterName;
	core::String _currentFilterLicense;
	voxelcollection::VoxelFile _selected;
	video::TexturePoolPtr _texturePool;

	int buildVoxelTree(const voxelcollection::VoxelFiles &voxelFiles);

	bool filtered(const voxelcollection::VoxelFile &voxelFile) const;
	void updateFilters();
	bool isFilterActive() const;

public:
	CollectionPanel(ui::IMGUIApp *app, const video::TexturePoolPtr &texturePool);
	virtual ~CollectionPanel();

	bool init();
	int update(const voxelcollection::VoxelFileMap &voxelFilesMap);
	void shutdown();

	video::TexturePtr thumbnailLookup(const voxelcollection::VoxelFile &voxelFile);
	voxelcollection::VoxelFile &selected();
};

} // namespace voxelcollection
