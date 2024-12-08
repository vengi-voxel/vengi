/**
 * @file
 */

#include "TextureLookup.h"
#include "app/App.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/Archive.h"
#include "io/File.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"

namespace voxelformat {

/**
 * @brief Tries to find a texture that matches the given not-yet-found texture name somewhere in the search path or in
 * some directory relative to the given reference file. It can also handle inputs without extensions - we apply the
 * extensions for all supported image files that could serve as textures here.
 *
 * TODO: VOXELFORMAT: use io::Archive here, too
 */
core::String lookupTexture(const core::String &referenceFile, const core::String &notFoundTexture,
						   const io::ArchivePtr &archive) {
	const core::String &additionalSearchPath = core::Var::getSafe(cfg::VoxformatTexturePath)->strVal();

	const core::String &referencePath = core::string::extractDir(referenceFile);
	core::String textureName = notFoundTexture;
	io::normalizePath(textureName);

	if (!core::string::isAbsolutePath(textureName) && !archive->exists(textureName)) {
		textureName = core::string::path(referencePath, textureName);
	}

	if (archive->exists(textureName)) {
		Log::debug("Found image %s in path %s", notFoundTexture.c_str(), textureName.c_str());
		return textureName;
	}

	const io::FilesystemPtr &fs = io::filesystem();
	if (!referencePath.empty()) {
		// change the current working dir to the reference path to let relative paths work
		fs->sysPushDir(referencePath);
	}
	const core::String &filenameWithExt = core::string::extractFilenameWithExtension(textureName);
	const core::String &path = core::string::extractDir(textureName);
	core::String fullpath = io::searchPathFor(fs, path, filenameWithExt);
	if (fullpath.empty() && path != referencePath) {
		fullpath = io::searchPathFor(fs, referencePath, filenameWithExt);
	}
	if (!additionalSearchPath.empty() && fullpath.empty()) {
		fullpath = io::searchPathFor(fs, additionalSearchPath, filenameWithExt);
		Log::debug("Searching for texture %s in %s", filenameWithExt.c_str(), additionalSearchPath.c_str());
	}
	if (fullpath.empty()) {
		fullpath = io::searchPathFor(fs, "texture", filenameWithExt);
	}
	if (fullpath.empty()) {
		fullpath = io::searchPathFor(fs, "textures", filenameWithExt);
	}

	// if not found, loop over all supported image formats and repeat the search
	if (fullpath.empty()) {
		const core::String &baseFilename = core::string::extractFilename(textureName);
		if (!baseFilename.empty()) {
			for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
				for (const core::String &ext : desc->exts) {
					const core::String &f = core::string::format("%s.%s", baseFilename.c_str(), ext.c_str());
					if (f == filenameWithExt) {
						continue;
					}
					fullpath = io::searchPathFor(fs, path, f);
					if (fullpath.empty() && path != referencePath) {
						fullpath = io::searchPathFor(fs, referencePath, f);
					}
					if (!additionalSearchPath.empty() && fullpath.empty()) {
						fullpath = io::searchPathFor(fs, additionalSearchPath, filenameWithExt);
					}
					if (fullpath.empty()) {
						fullpath = io::searchPathFor(fs, "texture", f);
					}
					if (fullpath.empty()) {
						fullpath = io::searchPathFor(fs, "textures", f);
					}
					if (!fullpath.empty()) {
						if (!referencePath.empty()) {
							fs->sysPopDir();
						}
						return fullpath;
					}
				}
			}
		}
	}

	if (fullpath.empty()) {
		Log::error("Failed to perform texture lookup for '%s' (filename: '%s', register additional search paths with "
				   "the cvar %s)",
				   textureName.c_str(), filenameWithExt.c_str(), cfg::VoxformatTexturePath);
	}
	if (!referencePath.empty()) {
		fs->sysPopDir();
	}
	return fullpath;
}

} // namespace voxelformat
