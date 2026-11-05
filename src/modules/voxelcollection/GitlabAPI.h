/**
 * @file
 * @li https://docs.gitlab.com/ee/api/repositories.html
 * @li https://gitlab.com/api/v4/projects/<projectid>/repository/archive.zip?path=assets/
 * @li https://gitlab.com/api/v4/projects/<projectid>/repository/tree?path=assets/&recursive=true&per_page=1000
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"

namespace voxelcollection {
namespace gitlab {

struct TreeEntry {
	core::String path;
	core::String url;
};

core::String treeCacheFile(const core::String &repository, const core::String &branch, int page = 1);
core::String downloadUrl(const core::String &repository, const core::String &branch, const core::String &path);
core::DynamicArray<TreeEntry> reposGitTrees(const io::ArchivePtr &archive, const core::String &projectId,
											const core::String &branch, const core::String &path = "",
											bool force = false);

} // namespace gitlab
} // namespace voxelcollection
