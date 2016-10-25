#include "VoxLoader.h"
#include "io/FileStream.h"
#include "core/Common.h"

namespace voxel {

#define wrap(read) \
	if (read != 0) { \
		Log::error("Could not load vox file: Not enough data in stream"); \
		return nullptr; \
	}

RawVolume* VoxLoader::load(const io::FilePtr& file) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load vox file: File doesn't exist");
		return nullptr;
	}
	io::FileStream stream(file.get());
	uint32_t header;
	wrap(stream.readInt(header))
	constexpr uint32_t headerMagic = FourCC('V','O','X',' ');
	if (header != headerMagic) {
		Log::error("Could not load vox file: Invalid magic found (%u vs %u)", header, headerMagic);
		return nullptr;
	}

	uint32_t version;
	wrap(stream.readInt(version))

	if (version != 150) {
		Log::warn("Vox file loading is only tested for version 150 - but we've found %i", version);
	}

	/**
	 * 2. Chunk Structure
	 * -------------------------------------------------------------------------------
	 * # Bytes  | Type       | Value
	 * -------------------------------------------------------------------------------
	 * 1x4      | char       | chunk id
	 * 4        | int        | num bytes of chunk content (N)
	 * 4        | int        | num bytes of children chunks (M)
	 * N        |            | chunk content
	 * M        |            | children chunks
	 * -------------------------------------------------------------------------------
	 */

	uint32_t mainChunk;
	wrap(stream.readInt(mainChunk))
	if (mainChunk != FourCC('M','A','I','N')) {
		Log::error("Could not load vox file: Invalid magic for main chunk found");
		return nullptr;
	}

	uint32_t numBytesMainChunk;
	wrap(stream.readInt(numBytesMainChunk))
	uint32_t numBytesChildrenChunks;
	wrap(stream.readInt(numBytesChildrenChunks))

	Log::info("main => %i. children => %i", numBytesMainChunk, numBytesChildrenChunks);

	if (stream.remaining() < numBytesChildrenChunks) {
		Log::error("Could not load vox file: Incomplete file");
		return nullptr;
	}

	do {
		uint8_t byte;
		if (stream.readByte(byte) != 0) {
			break;
		}
	} while (stream.remaining() > 0);

	return nullptr;
}

}
