/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"

namespace voxelcollection {
namespace cubzh {

struct TreeEntry {
	core::String id;
	core::String repo;
	core::String name;
	int likes;
	core::String created;
	core::String updated;
	core::String category;
	core::String url;
};

core::String downloadUrl(const core::String &repo, const core::String &name);
core::DynamicArray<TreeEntry> repoList(const io::ArchivePtr &archive, const core::String &tk,
									   const core::String &usrId);

} // namespace cubzh
} // namespace voxelcollection
