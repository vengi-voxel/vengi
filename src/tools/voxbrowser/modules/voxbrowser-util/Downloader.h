/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicStringMap.h"

namespace voxbrowser {

struct VoxelSourceGithub {
	core::String repo;
	core::String commit;
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
	VoxelSourceSingle single;
};

struct VoxelFile {
	core::String source;
	core::String name;
	core::String license;
	core::String licenseUrl;
	core::String thumbnailUrl;
	core::String url;
	bool downloaded = false;

	core::String targetFile() const {
		return core::string::path(core::string::cleanPath(source), name);
	}

	core::String targetDir() const {
		return core::string::path(core::string::cleanPath(source), core::string::extractPath(name));
	}

	bool operator==(const VoxelFile &other) const {
		return source == other.source && name == other.name;
	}
};

using VoxelFiles = core::DynamicArray<voxbrowser::VoxelFile>;
using VoxelFileMap = core::DynamicStringMap<VoxelFiles>;

class Downloader {
public:
	core::DynamicArray<VoxelSource> sources();

	core::DynamicArray<VoxelFile> resolve(const VoxelSource &source) const;
};

} // namespace voxbrowser
