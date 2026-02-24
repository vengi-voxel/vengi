/**
 * @file
 */

#include "TextureLookup.h"
#include "app/App.h"
#include "core/Path.h"
#include "core/String.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/FormatDescription.h"

namespace voxelformat {

static core::Path searchInPath(const core::Path &file, const io::ArchivePtr &archive) {
	if (archive->exists(file)) {
		return file;
	}
	for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
		for (const core::String &ext : desc->exts) {
			const core::Path &f = file.replaceExtension(ext);
			if (archive->exists(f)) {
				return f;
			}
		}
	}
	Log::debug("Could not find texture %s", file.c_str());
	return {};
}

static core::Path searchInPath(const core::Path &referencePath, const core::Path &file, const io::ArchivePtr &archive) {
	core::Path result = searchInPath(file, archive);
	if (result.valid()) {
		return result;
	}
	if (file.isRelativePath()) {
		result = searchInPath(referencePath.append(file), archive);
		if (result.valid()) {
			return result;
		}
		core::Path copy(file);
		while (copy.hasParentDirectory()) {
			copy = copy.popFront();
			result = searchInPath(referencePath.append(copy), archive);
			if (result.valid()) {
				return result;
			}
		}
	} else {
		result = searchInPath(file.basename(), archive);
		if (result.valid()) {
			return result;
		}
		if (file.hasParentDirectory()) {
			result = searchInPath(referencePath, file.popFront(), archive);
			if (result.valid()) {
				return result;
			}
		}
	}
	return {};
}

core::Path lookupTexture(const core::Path &referenceFile, const core::Path &file, const io::ArchivePtr &archive, const core::DynamicArray<core::Path> &additionalSearchPaths) {
	const core::Path referencePath(referenceFile.dirname());
	core::Path foundFile = searchInPath(referencePath, file, archive);
	if (foundFile.valid()) {
		return foundFile;
	}
	for (const core::Path &searchPath : additionalSearchPaths) {
		foundFile = searchInPath(referencePath, searchPath + file, archive);
		if (foundFile.valid()) {
			return foundFile;
		}
	}
	const core::Path additionalSearchPath(core::Var::getVar(cfg::VoxformatTexturePath)->strVal());
	if (additionalSearchPath.valid()) {
		foundFile = searchInPath(additionalSearchPath, file, archive);
		if (foundFile.valid()) {
			return foundFile;
		}
	}
	Log::error("Could not find texture %s", file.c_str());
	return file;
}

} // namespace voxelformat
