/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "video/TexturePool.h"
#include "voxelcollection/CollectionManager.h"
#include "voxelcollection/Downloader.h"

#define TITLE_ASSET_LIST "Assets##list"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class CollectionPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	core::DynamicArray<io::FormatDescription> _filterEntries;
	SceneManagerPtr _sceneMgr;
	voxelcollection::CollectionManagerPtr _collectionMgr;
	float _filterFormatTextWidth = -1.0f;
	int _currentFilterFormatEntry = -1;
	bool _thumbnails = true;
	core::String _localDir;
	core::String _currentFilterName;
	core::String _currentFilterLicense;
	voxelcollection::VoxelFile _selected;
	video::TexturePoolPtr _texturePool;

	bool import(voxelcollection::VoxelFile *voxelFile);
	void contextMenu(voxelcollection::VoxelFile *voxelFile);
	void handleDoubleClick(voxelcollection::VoxelFile *voxelFile);
	void thumbnailTooltip(voxelcollection::VoxelFile *&voxelFile);
	int buildVoxelTree(const voxelcollection::VoxelFiles &voxelFiles);

	bool filtered(const voxelcollection::VoxelFile &voxelFile) const;
	void updateFilters();
	bool isFilterActive() const;

public:
	CollectionPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr,
			   const voxelcollection::CollectionManagerPtr &collectionMgr, const video::TexturePoolPtr &texturePool);
	virtual ~CollectionPanel();

	bool init();
	int update();
	void shutdown();

	void setThumbnails(bool state);

	video::TexturePtr thumbnailLookup(const voxelcollection::VoxelFile &voxelFile);
	voxelcollection::VoxelFile &selected();
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

inline void CollectionPanel::setThumbnails(bool state) {
	_thumbnails = state;
}

} // namespace voxedit
