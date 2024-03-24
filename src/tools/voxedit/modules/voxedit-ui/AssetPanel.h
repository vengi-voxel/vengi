/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/collection/DynamicArray.h"
#include "ui/Panel.h"
#include "video/TexturePool.h"
#include "voxelcollection/CollectionManager.h"
#include "voxelcollection/ui/CollectionPanel.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class AssetPanel : public ui::Panel {
private:
	using Super = ui::Panel;

	void loadTextures(const core::String &dir);
	video::TexturePoolPtr _texturePool;
	io::FilesystemPtr _filesystem;
	SceneManagerPtr _sceneMgr;
	voxelcollection::CollectionManagerPtr _collectionMgr;
	voxelcollection::CollectionPanel _collectionPanel;
	int _currentSelectedModel = 0;

public:
	AssetPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr,
			   const voxelcollection::CollectionManagerPtr &collectionMgr, const video::TexturePoolPtr &texturePool, const io::FilesystemPtr &filesystem);
	bool init();
	void update(const char *title, bool sceneMode, command::CommandExecutionListener &listener);
	void shutdown();
};

} // namespace voxedit
