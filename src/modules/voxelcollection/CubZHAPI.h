/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "io/Filesystem.h"

namespace voxelcollection {
namespace cubzh {

struct TreeEntry {
	core::String repo;
	core::String name;
	core::String description;
	core::String updated;
	core::String created;
	core::String id;
	core::String url;
	int likes;
};

core::String downloadUrl(const core::String &repo, const core::String &name);
core::DynamicArray<TreeEntry> repoList(const io::FilesystemPtr &filesystem, const core::String &tk,
									   const core::String &usrId);

} // namespace cubzh
} // namespace voxelcollection
