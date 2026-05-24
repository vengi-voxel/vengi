/**
 * @file
 */

#include "StudioIOFormat.h"
#include "LDrawFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "io/ZipArchive.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {

namespace priv {

static const char *const kStudioPassword = "soho0909";

static io::ArchivePtr openStudioZipArchive(io::SeekableReadStream &stream) {
	io::ArchivePtr zipArchive = io::openZipArchive(&stream);
	if (zipArchive) {
		io::ArchiveFiles files;
		zipArchive->list("*.ldr", files);
		if (!files.empty()) {
			return zipArchive;
		}
		Log::debug("No ldr files found in unencrypted studio io archive - trying with password");
	}
	if (stream.seek(0) == -1) {
		return {};
	}
	zipArchive = io::openZipArchive(&stream, kStudioPassword);
	if (!zipArchive) {
		Log::error("Could not open studio io file as zip archive");
		return {};
	}
	return zipArchive;
}

static core::String findModelFile(const io::ArchivePtr &zipArchive, const char *name) {
	io::ArchiveFiles files;
	zipArchive->list("*.ldr", files);
	for (const io::FilesystemEntry &entry : files) {
		if (entry.name.toLower() == name) {
			return entry.fullPath;
		}
	}
	return {};
}

static void renameModelNode(scenegraph::SceneGraph &sceneGraph, const core::String &filename) {
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	if (node == nullptr) {
		return;
	}
	const core::String nodeName = node->name().toLower();
	if (nodeName == "model.ldr" || nodeName == "model2.ldr") {
		node->setName(core::string::extractFilename(filename));
	}
}

} // namespace priv

bool StudioIOFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
								scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	const io::ArchivePtr zipArchive = priv::openStudioZipArchive(*stream);
	if (!zipArchive) {
		return false;
	}
	const core::String modelFile = priv::findModelFile(zipArchive, "model.ldr");
	const core::String model2File = priv::findModelFile(zipArchive, "model2.ldr");
	if (modelFile.empty() && model2File.empty()) {
		Log::error("Could not find model.ldr in studio io file %s", filename.c_str());
		return false;
	}

	LDrawFormat ldraw;
	if (!modelFile.empty()) {
		Log::debug("Loading studio io model from %s", modelFile.c_str());
		if (ldraw.load(modelFile, zipArchive, sceneGraph, ctx)) {
			priv::renameModelNode(sceneGraph, filename);
			return true;
		}
		Log::debug("Failed to load %s - trying model2.ldr with embedded part definitions", modelFile.c_str());
		sceneGraph.clear();
	}

	if (model2File.empty()) {
		Log::error("Failed to load LDraw model from studio io file %s", filename.c_str());
		return false;
	}
	Log::debug("Loading studio io model from %s", model2File.c_str());
	if (!ldraw.load(model2File, zipArchive, sceneGraph, ctx)) {
		Log::error("Failed to load LDraw model from studio io file %s", filename.c_str());
		return false;
	}
	priv::renameModelNode(sceneGraph, filename);
	return true;
}

image::ImagePtr StudioIOFormat::loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
											   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return {};
	}
	const io::ArchivePtr zipArchive = priv::openStudioZipArchive(*stream);
	if (!zipArchive) {
		return {};
	}
	io::ArchiveFiles files;
	zipArchive->list("*.png", files);
	for (const io::FilesystemEntry &entry : files) {
		if (entry.name.toLower() != "thumbnail.png") {
			continue;
		}
		core::ScopedPtr<io::SeekableReadStream> thumbnailStream(zipArchive->readStream(entry.fullPath));
		if (!thumbnailStream) {
			Log::error("Could not load file %s", entry.fullPath.c_str());
			return {};
		}
		return image::loadImage(entry.name, *thumbnailStream, thumbnailStream->size());
	}
	Log::debug("Could not find thumbnail.png in the studio io archive");
	return {};
}

} // namespace voxelformat
