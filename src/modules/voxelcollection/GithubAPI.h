/**
 * @file
 * @li https://docs.github.com/en/rest/git/trees?apiVersion=2022-11-28#get-a-tree
 * @li https://api.github.com/repos/vengi-voxel/vengi/git/trees/master?recursive=1
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"

namespace voxelcollection {
namespace github {

struct TreeEntry {
	core::String path;
	core::String url;
	int size = -1;
};

core::String downloadUrl(const io::ArchivePtr &archive, const core::String &repository, const core::String &branch,
						 const core::String &path, int size);
core::DynamicArray<TreeEntry> reposGitTrees(const io::ArchivePtr &archive, const core::String &repository,
											const core::String &branch, const core::String &path = "");

} // namespace github
} // namespace voxelcollection
