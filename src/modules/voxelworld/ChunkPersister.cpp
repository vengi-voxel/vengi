/**
 * @file
 */

#include "ChunkPersister.h"
#include "core/ByteStream.h"
#include "core/Zip.h"
#include "core/Assert.h"
#include "core/Enum.h"
#include "core/Trace.h"
#include "core/Log.h"

namespace voxelworld {

#define WORLD_FILE_VERSION 2

bool ChunkPersister::saveCompressed(const voxel::PagedVolume::ChunkPtr& chunk, core::ByteStream& outStream) const {
	// save the stuff
	const voxel::Voxel* voxelBuf = chunk->data();
	const int voxelSize = chunk->dataSizeInBytes();
	uint32_t neededVoxelBufLen = core::zip::compressBound(voxelSize);
	uint8_t* compressedVoxelBuf = new uint8_t[neededVoxelBufLen];
	std::unique_ptr<uint8_t[]> smartBuf(compressedVoxelBuf);
	size_t finalBufferSize;
	{
		core_trace_scoped(ChunkPersisterCompress);
		const bool success = core::zip::compress((const uint8_t*)voxelBuf, voxelSize, compressedVoxelBuf, neededVoxelBufLen, &finalBufferSize);
		if (!success) {
			Log::error("Failed to compress the voxel data");
			return false;
		}
	}
	{
		core_trace_scoped(ChunkPersisterSaveCompressed);
		outStream.addInt(voxelSize);
		outStream.addByte(WORLD_FILE_VERSION);
		outStream.append(compressedVoxelBuf, finalBufferSize);
	}
	return true;
}

bool ChunkPersister::loadCompressed(const voxel::PagedVolume::ChunkPtr& chunk, const uint8_t *fileBuf, size_t fileLen) const {
	core_trace_scoped(ChunkPersisterLoadCompressed);
	const size_t headerSize = sizeof(int32_t) + sizeof(uint8_t);
	if (!fileBuf || fileLen <= headerSize) {
		return false;
	}
	core::ByteStream bs(headerSize);
	bs.append(fileBuf, headerSize);
	const int len = bs.readInt();
	const int version = bs.readByte();

	if (version != WORLD_FILE_VERSION) {
		Log::warn("chunk has a wrong version number %i (expected %i)",
				version, WORLD_FILE_VERSION);
		return false;
	}
	const int sizeLimit = chunk->dataSizeInBytes();
	if (len != sizeLimit) {
		Log::error("extracted memory would not fit the target chunk (%i bytes vs %i chunk size)", len, sizeLimit);
		return false;
	}
	const uint8_t* buf = fileBuf + headerSize;
	const size_t remaining = fileLen - headerSize;

	// TODO: doesn't work on big endian
	uint8_t *targetBuf = (uint8_t*)chunk->data();
	if (!core::zip::uncompress(buf, remaining, targetBuf, sizeLimit)) {
		Log::error("Failed to uncompress the world data with len %i", len);
		return false;
	}
	return true;
}

}
