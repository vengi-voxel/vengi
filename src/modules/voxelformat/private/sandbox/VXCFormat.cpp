/**
 * @file
 */

#include "VXCFormat.h"
#include "VXRFormat.h"
#include "VXCArchive.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "io/Archive.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"
#include "io/ZipReadStream.h"
#include "scenegraph/SceneGraph.h"
#include "voxelformat/Format.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load vxc file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load vxc file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

bool VXCFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> in(archive->readStream(filename));
	if (!in) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	io::ZipReadStream stream(*in, (int)in->size());
	uint8_t magic[4];
	wrap(stream.readUInt8(magic[0]))
	wrap(stream.readUInt8(magic[1]))
	wrap(stream.readUInt8(magic[2]))
	wrap(stream.readUInt8(magic[3]))
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'C') {
		Log::error("Could not load vxc file: Invalid magic found (%c%c%c%c)", magic[0], magic[1], magic[2], magic[3]);
		return false;
	}
	int version = magic[3] - '0';
	if (version != 1) {
		Log::error("Could not load vxc file: Unsupported version found (%i)", version);
		return false;
	}

	core::SharedPtr<VXCArchive> vxcArchive = core::make_shared<VXCArchive>(stream);
	io::ArchiveFiles files;
	vxcArchive->list(".vxr", files);
	for (const io::FilesystemEntry &entry : files) {
		VXRFormat f;
		f.load(entry.name, vxcArchive, sceneGraph, ctx);
	}
	sceneGraph.updateTransforms();
	return !sceneGraph.empty();
}

bool VXCFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	return false;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
