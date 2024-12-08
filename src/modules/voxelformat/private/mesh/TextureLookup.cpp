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
#include "io/FormatDescription.h"

namespace voxelformat {

// TODO: VOXELFORMAT: use io::Archive here, too
core::String lookupTexture(const core::String &meshFilename, const core::String &in, const io::ArchivePtr &) {
	const core::String &meshPath = core::string::extractDir(meshFilename);
	core::String name = in;
	io::normalizePath(name);

	const io::FilesystemPtr &fs = io::filesystem();
	if (!core::string::isAbsolutePath(name) && !fs->exists(name)) {
		name = core::string::path(meshPath, name);
	}

	if (fs->exists(name)) {
		Log::debug("Found image %s in path %s", in.c_str(), name.c_str());
		return name;
	}

	const core::String &additionalSearchPath = core::Var::getSafe(cfg::VoxformatTexturePath)->strVal();

	if (!meshPath.empty()) {
		fs->sysPushDir(meshPath);
	}
	core::String filename = core::string::extractFilenameWithExtension(name);
	const core::String &path = core::string::extractDir(name);
	core::String fullpath = io::searchPathFor(fs, path, filename);
	if (fullpath.empty() && path != meshPath) {
		fullpath = io::searchPathFor(fs, meshPath, filename);
	}
	if (!additionalSearchPath.empty() && fullpath.empty()) {
		fullpath = io::searchPathFor(fs, additionalSearchPath, filename);
		Log::debug("Searching for texture %s in %s", filename.c_str(), additionalSearchPath.c_str());
	}
	if (fullpath.empty()) {
		fullpath = io::searchPathFor(fs, "texture", filename);
	}
	if (fullpath.empty()) {
		fullpath = io::searchPathFor(fs, "textures", filename);
	}

	// if not found, loop over all supported image formats and repeat the search
	if (fullpath.empty()) {
		const core::String &baseFilename = core::string::extractFilename(name);
		if (!baseFilename.empty()) {
			for (const io::FormatDescription *desc = io::format::images(); desc->valid(); ++desc) {
				for (const core::String &ext : desc->exts) {
					const core::String &f = core::string::format("%s.%s", baseFilename.c_str(), ext.c_str());
					if (f == filename) {
						continue;
					}
					fullpath = io::searchPathFor(fs, path, f);
					if (fullpath.empty() && path != meshPath) {
						fullpath = io::searchPathFor(fs, meshPath, f);
					}
					if (!additionalSearchPath.empty() && fullpath.empty()) {
						fullpath = io::searchPathFor(fs, additionalSearchPath, filename);
					}
					if (fullpath.empty()) {
						fullpath = io::searchPathFor(fs, "texture", f);
					}
					if (fullpath.empty()) {
						fullpath = io::searchPathFor(fs, "textures", f);
					}
					if (!fullpath.empty()) {
						if (!meshPath.empty()) {
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
				   name.c_str(), filename.c_str(), cfg::VoxformatTexturePath);
	}
	if (!meshPath.empty()) {
		fs->sysPopDir();
	}
	return fullpath;
}

} // namespace voxelformat
