/**
 * @file
 */

#pragma once

#include "core/collection/ConcurrentQueue.h"
#include "ui/IMGUIApp.h"
#include "video/TexturePool.h"
#include "voxelcollection/Downloader.h"

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
	core::ConcurrentQueue<voxelcollection::VoxelFile> _newVoxelFiles;
	core::ConcurrentQueue<image::ImagePtr> _imageQueue;
	voxelcollection::VoxelFileMap _voxelFilesMap;
	video::TexturePool _texturePool;
	core::AtomicInt _downloadProgress = 0; // 0-100
	int _count = 0;

	void loadThumbnail(const voxelcollection::VoxelFile &voxelFile);
	void downloadAll();
	void thumbnailAll();
protected:
	void printUsageHeader() const override;

public:
	VoxBrowser(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	void onRenderUI() override;

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
	app::AppState onRunning() override;
};
