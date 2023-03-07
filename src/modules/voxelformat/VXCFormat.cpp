/**
 * @file
 */

#include "VXCFormat.h"
#include "app/App.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "app/App.h"
#include "io/ZipReadStream.h"
#include "scenegraph/SceneGraph.h"
#include "voxelformat/VXMFormat.h"
#include "voxelformat/VXRFormat.h"

namespace voxelformat {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load vxc file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) != true) { \
		Log::error("Could not load vxc file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

bool VXCFormat::loadGroups(const core::String &filename, io::SeekableReadStream &in, SceneGraph &sceneGraph, const LoadContext &ctx) {
	io::ZipReadStream stream(in, (int)in.size());
	uint8_t magic[4];
	wrap(stream.readUInt8(magic[0]))
	wrap(stream.readUInt8(magic[1]))
	wrap(stream.readUInt8(magic[2]))
	wrap(stream.readUInt8(magic[3]))
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'C') {
		Log::error("Could not load vxc file: Invalid magic found (%c%c%c%c)",
			magic[0], magic[1], magic[2], magic[3]);
		return false;
	}
	int version = magic[3] - '0';
	if (version != 1) {
		Log::error("Could not load vxc file: Unsupported version found (%i)", version);
		return false;
	}

	uint32_t entries;
	wrap(stream.readUInt32(entries))
	core::String vxr;
	for (uint32_t i = 0; i < entries; ++i) {
		char path[1024];
		wrapBool(stream.readString(sizeof(path), path, true))
		uint32_t fileSize;
		wrap(stream.readUInt32(fileSize))
		io::BufferedReadWriteStream substream(stream, fileSize);
		substream.seek(0);
		// TODO: currently we need them as files - we don't have a virtual filesystem that could deal with this yet...
		io::filesystem()->write(path, substream.getBuffer(), substream.size());
		const core::String &ext = core::string::extractExtension(path);
		if (ext == "vxr") {
			vxr = path;
		}
	}

	if (!vxr.empty()) {
		VXRFormat f;
		io::FilePtr vxrFile = io::filesystem()->open(vxr);
		if (vxrFile->validHandle()) {
			io::FileStream fstream(vxrFile);
			f.load(vxr, fstream, sceneGraph, ctx);
		}
	}
	sceneGraph.updateTransforms();
	return !sceneGraph.empty();
}

bool VXCFormat::saveGroups(const SceneGraph &sceneGraph, const core::String &filename, io::SeekableWriteStream &stream, const SaveContext &ctx) {
	return false;
}

#undef wrap
#undef wrapBool

} // namespace voxel
