/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/collection/ConcurrentQueue.h"
#include "core/collection/StringSet.h"
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
	core::ConcurrentQueue<VoxelFile> _newVoxelFiles;
	VoxelFileMap _voxelFilesMap;

	core::ConcurrentQueue<image::ImagePtr> _imageQueue;
	video::TexturePoolPtr _texturePool;

	core::AtomicInt _downloadProgress = 0; // 0-100
	core::AtomicBool _shouldQuit = false;
	int _count = 0;

	std::future<void> _local;
	core::String _localDir;

	core::StringSet _onlineResolvedSources;
	std::future<VoxelFiles> _onlineResolve;
	VoxelSources _sources;
	std::future<VoxelSources> _onlineSources;

public:
	CollectionManager(const io::FilesystemPtr &filesystem, const video::TexturePoolPtr &texturePool);
	virtual ~CollectionManager();

	bool init() override;
	/**
	 * @param[in] n The amount of new voxel file instances to add in one update() call
	 */
	void update(double nowSeconds, int n = 100);
	void shutdown() override;

	bool setLocalDir(const core::String &dir);

	bool local();
	bool online(bool resolve = true);
	/**
	 * @brief Blocks until all local files are loaded.
	 * @sa local()
	 */
	void waitLocal();
	/**
	 * @brief Blocks until all queued online sources are loaded.
	 * @sa online()
	 */
	void waitOnline();
	void resolve(const VoxelSource &source);
	bool resolved(const VoxelSource &source) const;

	void loadThumbnail(const VoxelFile &voxelFile);
	void thumbnailAll();
	void downloadAll();
	bool download(VoxelFile &voxelFile);

	const VoxelFileMap &voxelFilesMap() const;
	const VoxelSources &sources() const;
	int downloadProgress() const;
	int allEntries() const;
};

inline const VoxelSources &CollectionManager::sources() const {
	return _sources;
}

typedef core::SharedPtr<CollectionManager> CollectionManagerPtr;

}; // namespace voxelcollection
