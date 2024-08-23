/**
 * @file
 */

#pragma once

#include "GithubAPI.h"
#include "GitlabAPI.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicStringMap.h"
#include "io/Archive.h"

namespace voxelcollection {

struct VoxelSourceGithub {
	core::String repo;
	core::String commit;
	// limit the repository to a specific path - or if empty, search the whole repository for supported files
	core::String path;
	core::String license;
	bool enableMeshes = false;
};

struct VoxelSourceGitlab {
	core::String repo;
	core::String commit;
	// limit the repository to a specific path - or if empty, search the whole repository for supported files
	core::String path;
	core::String license;
	bool enableMeshes = false;
};

struct VoxelSourceSingle {
	core::String url;
};

#define LOCAL_SOURCE "local"

struct VoxelSource {
	core::String name;
	core::String license;
	core::String thumbnail;
	core::String provider;
	VoxelSourceGithub github;
	VoxelSourceGitlab gitlab;
	VoxelSourceSingle single;

	inline bool isLocal() const {
		return name == LOCAL_SOURCE;
	}
};

class VoxelFile {
public:
	core::String source;
	// the name of the fill - including extension, without directory
	core::String name;
	// the full path relative to the voxel source including any directory
	// except for the local source - where is the full path to the file on
	// the local filesystem
	core::String fullPath;
	core::String license;
	core::String licenseUrl;
	core::String thumbnailUrl;
	core::String url;
	bool downloaded = false;

	// return the target file path relative to the voxel source with the source and directory name
	core::String targetFile() const;
	// return the target directory relative to the voxel source with the source and directory name - but without the
	// filename
	core::String targetDir() const;

	inline bool isLocal() const {
		return source == "local";
	}

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
protected:
	bool handleArchive(const io::ArchivePtr &archive, const VoxelFile &file, VoxelFiles &files,
					   core::AtomicBool &shouldQuit) const;

	bool handleFile(const io::ArchivePtr &archive, core::AtomicBool &shouldQuit, VoxelFiles &files, VoxelFile &file,
					bool enableMeshes) const;

public:
	VoxelSources sources();
	VoxelSources sources(const core::String &json);
	VoxelFiles resolve(const io::ArchivePtr &archive, const VoxelSource &source, core::AtomicBool &shouldQuit) const;

	bool download(const io::ArchivePtr &archive, const VoxelFile &file) const;

	core::DynamicArray<VoxelFile> processEntries(const core::DynamicArray<github::TreeEntry> &entries,
												 const VoxelSource &source, const io::ArchivePtr &archive,
												 core::AtomicBool &shouldQuit) const;
	core::DynamicArray<VoxelFile> processEntries(const core::DynamicArray<gitlab::TreeEntry> &entries,
												 const VoxelSource &source, const io::ArchivePtr &archive,
												 core::AtomicBool &shouldQuit) const;
};

} // namespace voxelcollection
