/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicStringMap.h"
#include "io/Filesystem.h"

namespace voxbrowser {

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
	core::String name;
	core::String fullPath;
	core::String license;
	core::String licenseUrl;
	core::String thumbnailUrl;
	core::String url;
	bool downloaded = false;

	core::String targetFile() const;
	core::String targetDir() const;

	bool operator==(const VoxelFile &other) const {
		return source == other.source && name == other.name;
	}
};

using VoxelFiles = core::DynamicArray<VoxelFile>;
using VoxelFileMap = core::DynamicStringMap<VoxelFiles>;

class Downloader {
private:
	void handleArchive(const io::FilesystemPtr &filesystem, const VoxelFile &file, core::DynamicArray<VoxelFile> &files) const;

public:
	core::DynamicArray<VoxelSource> sources();

	core::DynamicArray<VoxelFile> resolve(const io::FilesystemPtr &filesystem, const VoxelSource &source) const;

	bool download(const io::FilesystemPtr &filesystem, const VoxelFile &file) const;
};

} // namespace voxbrowser
