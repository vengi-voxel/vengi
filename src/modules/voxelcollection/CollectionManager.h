/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/SharedPtr.h"
#include "core/collection/ConcurrentQueue.h"
#include "core/collection/StringSet.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Future.h"
#include "io/Archive.h"
#include "io/Filesystem.h"
#include "video/Texture.h"
#include "video/TexturePool.h"
#include "voxelcollection/Downloader.h"

namespace voxelcollection {

class CollectionManager : public core::IComponent {
private:
	io::ArchivePtr _archive;

	using QueuePtr = core::SharedPtr<core::ConcurrentQueue<VoxelFile>>;
	QueuePtr _newVoxelFiles;
	VoxelFileMap _voxelFilesMap;

	using ImageQueuePtr = core::SharedPtr<core::ConcurrentQueue<image::ImagePtr>>;
	ImageQueuePtr _imageQueue;
	video::TexturePoolPtr _texturePool;
	io::FilesystemPtr _filesystem;

	core::AtomicBool _shouldQuit = false;
	int _count = 0;

	core::Future<void> _local;
	core::String _localDir;

	core::StringSet _onlineResolvedSources;
	core::Future<VoxelFiles> _onlineResolve;
	VoxelSources _sources;
	core::Future<VoxelSources> _onlineSources;
	static bool download(const io::ArchivePtr &archive, VoxelFile &voxelFile);

public:
	CollectionManager(const io::FilesystemPtr &filesystem, const video::TexturePoolPtr &texturePool);
	virtual ~CollectionManager();

	bool init() override;
	/**
	 * @param[in] n The amount of new voxel file instances to add in one update() call
	 */
	void update(double nowSeconds, int n = 100);
	void shutdown() override;

	const core::String &localDir() const;
	bool setLocalDir(const core::String &dir);

	bool local(bool wait = false);
	bool online(bool wait = false);
	void resolve(const VoxelSource &source, bool async = true);
	bool resolved(const VoxelSource &source) const;

	/**
	 * @brief Load existing thumbnails - either from png files or from the voxel format file itself (if supported)
	 * @note This does NOT create thumbnails from vengi render shots
	 */
	void loadThumbnail(const VoxelFile &voxelFile);
	bool createThumbnail(const VoxelFile &voxelFile);

	bool download(VoxelFile &voxelFile);

	const VoxelFileMap &voxelFilesMap() const;
	const VoxelSources &sources() const;
	int allEntries() const;

	core::String absolutePath(const VoxelFile &voxelFile) const;
};

inline const core::String &CollectionManager::localDir() const {
	return _localDir;
}

inline const VoxelSources &CollectionManager::sources() const {
	return _sources;
}

typedef core::SharedPtr<CollectionManager> CollectionManagerPtr;

}; // namespace voxelcollection
