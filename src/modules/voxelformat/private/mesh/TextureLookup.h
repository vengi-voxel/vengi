/**
 * @file
 */

#pragma once

#include "core/Path.h"
#include "core/String.h"
#include "io/Archive.h"

namespace voxelformat {

/**
 * @brief Tries to find a texture that matches the given not-yet-found texture name somewhere in the search path or in
 * some directory relative to the given reference file. It can also handle inputs without extensions - we apply the
 * extensions for all supported image files that could serve as textures here.
 */
core::Path lookupTexture(const core::Path &referenceFile, const core::Path &file, const io::ArchivePtr &archive,
						 const core::DynamicArray<core::Path> &additionalSearchPaths = {});

inline core::String lookupTexture(const core::String &referenceFile, const core::String &file,
								  const io::ArchivePtr &archive,
								  const core::DynamicArray<core::Path> &additionalSearchPaths = {}) {
	const core::Path &path = lookupTexture(core::Path(referenceFile), core::Path(file), archive, additionalSearchPaths);
	return path.lexicallyNormal();
}

} // namespace voxelformat
