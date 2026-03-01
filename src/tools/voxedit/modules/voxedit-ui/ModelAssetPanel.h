/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "io/FormatDescription.h"
#include "ui/Panel.h"
#include "video/TexturePool.h"
#include "voxedit-util/SceneManager.h"
#include "voxelcollection/CollectionManager.h"
#include "voxelcollection/Downloader.h"

namespace voxedit {

class ModelAssetPanel : public ui::Panel {
private:
	using Super = ui::Panel;

	core::DynamicArray<io::FormatDescription> _filterEntries;
	SceneManagerPtr _sceneMgr;
	voxelcollection::CollectionManagerPtr _collectionMgr;
	float _filterFormatTextWidth = -1.0f;
	int _currentFilterFormatEntry = -1;
	bool _thumbnails = true;
	core::String _currentFilterName;
	core::String _currentFilterLicense;
	core::String _dragAndDropModel;
	voxelcollection::VoxelFile _selected;
	video::TexturePoolPtr _texturePool;

	bool import(voxelcollection::VoxelFile *voxelFile);
	void contextMenu(voxelcollection::VoxelFile *voxelFile);
	void handleDoubleClick(voxelcollection::VoxelFile *voxelFile);
	void thumbnailTooltip(voxelcollection::VoxelFile *voxelFile);
	void handleDragAndDrop(int row, voxelcollection::VoxelFile *voxelFile);
	int buildVoxelTree(const voxelcollection::VoxelFiles &voxelFiles);

	bool filtered(const voxelcollection::VoxelFile &voxelFile) const;
	void updateFilters();
	bool isFilterActive() const;
	video::TexturePtr thumbnailLookup(const voxelcollection::VoxelFile &voxelFile);
	voxelcollection::VoxelFile &selected();

public:
	ModelAssetPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr,
					const voxelcollection::CollectionManagerPtr &collectionMgr, const video::TexturePoolPtr &texturePool);
	bool init();
	void update(const char *id, command::CommandExecutionListener &listener);
	void shutdown();
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

inline voxelcollection::VoxelFile &ModelAssetPanel::selected() {
	return _selected;
}

} // namespace voxedit
