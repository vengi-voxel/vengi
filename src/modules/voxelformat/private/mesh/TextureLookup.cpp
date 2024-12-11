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
 * @brief Searches a filename in any a given path.
 * @param[in] path The path could be given as absolute non existing path like e.g. @c /foo/bar/textures/
 * where the given @c filename doesn't exists. But a directory relative to the current working dir named
 * @c textures with the given file exists. This will resolve the case insensitive lookup for you.
 */
static core::String searchPathFor(const io::FilesystemPtr &filesystem, const core::String &path,
								  const core::String &filename) {
	if (filename.empty()) {
		Log::warn("No filename given to perform lookup in '%s'", path.c_str());
		return "";
	}
	core::DynamicArray<core::String> tokens;
	core::string::splitString(path, tokens, "/");
	while (!tokens.empty()) {
		if (io::Filesystem::sysIsReadableDir(tokens[0])) {
			Log::trace("readable dir: %s", tokens[0].c_str());
			break;
		}
		Log::trace("not a readable dir: %s", tokens[0].c_str());
		tokens.erase(0);
		if (tokens.empty()) {
			break;
		}
	}
	core::String relativePath;
	for (const core::String &t : tokens) {
		relativePath = core::string::path(relativePath, t);
	}
	core::DynamicArray<io::FilesystemEntry> entities;
	const core::String abspath = filesystem->sysAbsolutePath(relativePath);
	filesystem->list(abspath, entities);
	Log::trace("Found %i entries in %s", (int)entities.size(), abspath.c_str());
	auto predicate = [&](const io::FilesystemEntry &e) { return core::string::iequals(e.name, filename); };
	auto iter = core::find_if(entities.begin(), entities.end(), predicate);
	if (iter == entities.end()) {
		Log::debug("Could not find %s in '%s'", filename.c_str(), abspath.c_str());
		for (const auto &e : entities) {
			Log::trace("* %s", e.name.c_str());
		}
		return "";
	}
	Log::debug("Found %s in %s", iter->name.c_str(), relativePath.c_str());
	return core::string::path(abspath, iter->name);
}

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
	const core::String cleanNotFoundTexture = core::string::sanitizePath(notFoundTexture);
	core::String textureName = cleanNotFoundTexture;
	io::normalizePath(textureName);

	if (!core::string::isAbsolutePath(textureName) && !archive->exists(textureName)) {
		textureName = core::string::path(referencePath, textureName);
	}

	if (archive->exists(textureName)) {
		Log::debug("Found image %s in path %s", notFoundTexture.c_str(), textureName.c_str());
		return core::string::sanitizePath(textureName);
	}

	const io::FilesystemPtr &fs = io::filesystem();
	const core::String &filenameWithExt = core::string::extractFilenameWithExtension(textureName);
	const core::String &path = core::string::extractDir(textureName);
	core::String fullpath = searchPathFor(fs, path, filenameWithExt);
	if (fullpath.empty() && path != referencePath) {
		fullpath = searchPathFor(fs, referencePath, filenameWithExt);
	}
	if (!additionalSearchPath.empty() && fullpath.empty()) {
		fullpath = searchPathFor(fs, additionalSearchPath, filenameWithExt);
		Log::debug("Searching for texture %s in %s", filenameWithExt.c_str(), additionalSearchPath.c_str());
	}
	if (fullpath.empty()) {
		fullpath = searchPathFor(fs, "texture", filenameWithExt);
	}
	if (fullpath.empty()) {
		fullpath = searchPathFor(fs, "textures", filenameWithExt);
	}

	// if not found, loop over all supported image formats and repeat the search
	if (fullpath.empty()) {
		const core::String &textureFilenameWithoutExt = core::string::extractFilename(textureName);
		// not yet found, try to replace the given extension to search for other images with the same
		// base name - but replace the extensions by iterating over all supported file format descriptions
		// for images.
		if (!textureFilenameWithoutExt.empty()) {
			for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
				for (const core::String &ext : desc->exts) {
					const core::String &f =
						core::string::format("%s.%s", textureFilenameWithoutExt.c_str(), ext.c_str());
					if (f == filenameWithExt) {
						continue;
					}
					fullpath = searchPathFor(fs, path, f);
					if (fullpath.empty() && path != referencePath) {
						fullpath = searchPathFor(fs, referencePath, f);
					}
					if (!additionalSearchPath.empty() && fullpath.empty()) {
						const core::String &nfext =
							core::string::format("%s.%s", cleanNotFoundTexture.c_str(), ext.c_str());
						fullpath = searchPathFor(fs, additionalSearchPath, nfext);
					}
					if (fullpath.empty()) {
						fullpath = searchPathFor(fs, "texture", f);
					}
					if (fullpath.empty()) {
						fullpath = searchPathFor(fs, "textures", f);
					}
					if (!fullpath.empty()) {
						if (!referencePath.empty()) {
							fs->sysPopDir();
						}
						return core::string::sanitizePath(fullpath);
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
	return core::string::sanitizePath(fullpath);
}

} // namespace voxelformat
