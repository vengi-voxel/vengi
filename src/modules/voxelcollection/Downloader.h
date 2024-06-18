/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicStringMap.h"
#include "io/Archive.h"
#include "io/Filesystem.h"

namespace voxelcollection {

struct VoxelSourceGithub {
	core::String repo;
	core::String commit;
	// limit the repository to a specific path - or if empty, search the whole repository for supported files
	core::String path;
	core::String license;
};

struct VoxelSourceGitlab {
	core::String repo;
	core::String commit;
	// limit the repository to a specific path - or if empty, search the whole repository for supported files
	core::String path;
	core::String license;
};

struct VoxelSourceSingle {
	core::String url;
};

struct VoxelSource {
	core::String name;
	core::String license;
	core::String thumbnail;
	core::String provider;
	VoxelSourceGithub github;
	VoxelSourceGitlab gitlab;
	VoxelSourceSingle single;
};

struct VoxelFile {
	core::String source;
	// the name of the fill - including extension, without directory
	core::String name;
	// the full path relative to the voxel source including any directory
	core::String fullPath;
	core::String license;
	core::String licenseUrl;
	core::String thumbnailUrl;
	core::String url;
	bool downloaded = false;

	core::String targetFile() const;
	core::String targetDir() const;

	inline bool operator<(const VoxelFile &other) const {
		return name < other.name;
	}

	inline bool operator==(const VoxelFile &other) const {
		return source == other.source && name == other.name;
	}
};

using VoxelFiles = core::DynamicArray<VoxelFile>;
struct VoxelCollection {
	VoxelFiles files;
	double timestamp = 0.0;
	bool sorted = false;
};
using VoxelFileMap = core::DynamicStringMap<VoxelCollection>;

using VoxelSources = core::DynamicArray<VoxelSource>;

class Downloader {
private:
	void handleArchive(const io::ArchivePtr &archive, const VoxelFile &file,
					  VoxelFiles &files, core::AtomicBool &shouldQuit) const;

public:
	VoxelSources sources();
	VoxelSources sources(const core::String &json);

	VoxelFiles resolve(const io::ArchivePtr &archive, const VoxelSource &source, core::AtomicBool &shouldQuit) const;

	bool download(const io::ArchivePtr &archive, const VoxelFile &file) const;
};

} // namespace voxelcollection
