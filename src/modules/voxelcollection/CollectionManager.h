/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/collection/ConcurrentQueue.h"
#include "core/concurrent/Atomic.h"
#include "io/Filesystem.h"
#include "video/Texture.h"
#include "video/TexturePool.h"
#include "voxelcollection/Downloader.h"
#include <future>

namespace voxelcollection {

class CollectionManager : public core::IComponent {
private:
	io::FilesystemPtr _filesystem;
	core::ConcurrentQueue<voxelcollection::VoxelFile> _newVoxelFiles;
	voxelcollection::VoxelFileMap _voxelFilesMap;

	core::ConcurrentQueue<image::ImagePtr> _imageQueue;
	video::TexturePoolPtr _texturePool;

	core::AtomicInt _downloadProgress = 0; // 0-100
	core::AtomicBool _shouldQuit = false;
	int _count = 0;
	std::future<void> _online;
	std::future<void> _local;

public:
	CollectionManager(const io::FilesystemPtr &filesystem, const video::TexturePoolPtr &texturePool);
	virtual ~CollectionManager();

	bool init() override;
	/**
	 * @param[in] n The amount of new voxel file instances to add in one update() call
	 */
	void update(double nowSeconds, int n = 100);
	void shutdown() override;

	void local();
	void online();

	void loadThumbnail(const voxelcollection::VoxelFile &voxelFile);
	void thumbnailAll();
	void downloadAll();
	bool download(VoxelFile &voxelFile);

	const voxelcollection::VoxelFileMap &voxelFilesMap() const;
	int downloadProgress() const;
	int allEntries() const;
};

typedef core::SharedPtr<CollectionManager> CollectionManagerPtr;

}; // namespace voxelcollection
