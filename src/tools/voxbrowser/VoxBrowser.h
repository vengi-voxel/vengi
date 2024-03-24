/**
 * @file
 */

#pragma once

#include "ui/IMGUIApp.h"
#include "video/TexturePool.h"
#include "voxelcollection/CollectionManager.h"

namespace voxbrowser {
class MainWindow;
}

/**
 * @brief This is a voxel browser that can download voxel files
 *
 * @ingroup Tools
 */
class VoxBrowser : public ui::IMGUIApp {
private:
	using Super = ui::IMGUIApp;
	voxbrowser::MainWindow *_mainWindow = nullptr;
	voxelcollection::CollectionManager _collectionMgr;
	video::TexturePoolPtr _texturePool;

protected:
	void printUsageHeader() const override;

public:
	VoxBrowser(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider, const video::TexturePoolPtr &texturePool);

	void onRenderUI() override;

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
	app::AppState onRunning() override;
};
