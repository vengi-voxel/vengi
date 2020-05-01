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

#define WORLD_FILE_VERSION 1

bool ChunkPersister::saveCompressed(const voxel::PagedVolume::ChunkPtr& chunk, core::ByteStream& outStream) const {
	core_trace_scoped(ChunkPersisterSaveCompressed);
	core::ByteStream voxelStream(chunk->dataSizeInBytes());
	const int width = chunk->sideLength();
	const int height = chunk->sideLength();
	const int depth = chunk->sideLength();

	for (int z = 0; z < depth; ++z) {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				const voxel::Voxel& voxel = chunk->voxel(x, y, z);
				static_assert(sizeof(voxel::VoxelType) == sizeof(uint8_t), "Voxel type size changed");
				voxelStream.addByte(core::enumVal(voxel.getMaterial()));
				voxelStream.addByte(voxel.getColor());
			}
		}
	}

	// save the stuff
	const uint8_t* voxelBuf = voxelStream.getBuffer();
	const int voxelSize = voxelStream.getSize();
	uint32_t neededVoxelBufLen = core::zip::compressBound(voxelSize);
	uint8_t* compressedVoxelBuf = new uint8_t[neededVoxelBufLen];
	size_t finalBufferSize;
	const bool success = core::zip::compress(voxelBuf, voxelSize, compressedVoxelBuf, neededVoxelBufLen, &finalBufferSize);
	std::unique_ptr<uint8_t[]> smartBuf(compressedVoxelBuf);
	if (!success) {
		Log::error("Failed to compress the voxel data");
		return false;
	}
	outStream.addFormat("ib", voxelSize, WORLD_FILE_VERSION);
	outStream.append(compressedVoxelBuf, finalBufferSize);
	return true;
}

bool ChunkPersister::loadCompressed(const voxel::PagedVolume::ChunkPtr& chunk, const uint8_t *fileBuf, size_t fileLen) const {
	core_trace_scoped(ChunkPersisterLoadCompressed);
	if (!fileBuf || fileLen <= 0) {
		return false;
	}
	core::ByteStream bs(fileLen);
	bs.append(fileBuf, fileLen);
	int len;
	int version;
	bs.readFormat("ib", &len, &version);

	if (version != WORLD_FILE_VERSION) {
		Log::error("chunk has a wrong version number %i (expected %i)",
				version, WORLD_FILE_VERSION);
		return false;
	}
	const int sizeLimit = 1024;
	if (len > 1000l * 1000l * sizeLimit) {
		Log::error("extracted memory would be more than %i MB",
				sizeLimit);
		return false;
	}

	uint8_t *targetBuf = new uint8_t[len];
	std::unique_ptr<uint8_t[]> smartTargetBuf(targetBuf);

	if (!core::zip::uncompress(bs.getBuffer(), bs.getSize(), targetBuf, len)) {
		Log::error("Failed to uncompress the world data with len %i", len);
		return false;
	}

	core::ByteStream voxelBuf(len);
	voxelBuf.append(targetBuf, len);

	const int width = chunk->sideLength();
	const int height = chunk->sideLength();
	const int depth = chunk->sideLength();

	for (int z = 0; z < depth; ++z) {
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				core_assert_msg(voxelBuf.getSize() >= 1,
						"Failed to load from buffer (x: %i, y: %i, z: %i)",
						x, y, z);
				static_assert(sizeof(voxel::VoxelType) == sizeof(uint8_t), "Voxel type size changed");
				const voxel::VoxelType material =
						(voxel::VoxelType) voxelBuf.readByte();
				const uint8_t colorIndex = voxelBuf.readByte();
				const voxel::Voxel &voxel = createVoxel(material, colorIndex);
				chunk->setVoxel(x, y, z, voxel);
			}
		}
	}
	return true;
}

}
