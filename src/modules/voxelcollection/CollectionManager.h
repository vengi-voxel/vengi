/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/SharedPtr.h"
#include "core/collection/ConcurrentQueue.h"
#include "core/collection/StringSet.h"
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
	using VoxelSourceQueuePtr = core::SharedPtr<core::ConcurrentQueue<VoxelSource>>;
	VoxelSourceQueuePtr _voxelSourceQueue;
	using ResolvedSourceQueuePtr = core::SharedPtr<core::ConcurrentQueue<core::String>>;
	ResolvedSourceQueuePtr _resolvedSourceQueue;
	video::TexturePoolPtr _texturePool;
	io::FilesystemPtr _filesystem;

	int _count = 0;

	core::String _localDir;

	core::StringSet _onlineResolvedSources;
	VoxelSources _sources;
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

	bool local();
	/**
	 * @brief Load the online voxel collection sources
	 * @param[in] force If @c true, ignore the cached browser-data json and re-download
	 */
	bool online(bool force = false);
	/**
	 * @brief Resolve a voxel source into voxel files
	 * @param[in] force If @c true, ignore cached provider json and re-download
	 */
	void resolve(const VoxelSource &source, bool async = true, bool force = false);
	bool resolved(const VoxelSource &source) const;
	/**
	 * @brief Whether a previously downloaded provider listing exists locally
	 */
	bool cached(const VoxelSource &source) const;
	/**
	 * @brief Clear online collections and re-download the source list
	 */
	void reloadOnline();
	/**
	 * @brief Re-download the cached provider json for a single source and re-resolve it
	 */
	void reload(const VoxelSource &source);

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
