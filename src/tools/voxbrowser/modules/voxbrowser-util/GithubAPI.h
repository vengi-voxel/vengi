/**
 * @file
 * @li https://docs.github.com/en/rest/git/trees?apiVersion=2022-11-28#get-a-tree
 * @li https://api.github.com/repos/vengi-voxel/vengi/git/trees/master?recursive=1
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace github {

struct TreeEntry {
	core::String path;
	core::String mode;
	core::String type;
	core::String sha;
	int size = 0;
	core::String url;
};

core::String downloadUrl(const core::String &repository, const core::String &branch, const core::String &path);
core::DynamicArray<TreeEntry> reposGitTrees(const core::String &repository, const core::String &branch);

} // namespace github
